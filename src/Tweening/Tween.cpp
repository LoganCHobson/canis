#include <Canis/Debug.hpp>
#include <Canis/Scene.hpp>
#include <Canis/Tweening/Shortcuts.hpp>
#include <Canis/Tweening/TweenWorld.hpp>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <map>
#include <stdexcept>
#include <vector>

namespace Canis::Tweening {
namespace {
constexpr double epsilon = 1e-9;
void Seconds(double v) {
  if (!std::isfinite(v) || v < 0)
    throw std::invalid_argument("Tween time must be finite and nonnegative");
}
void Validate(Value v) {
  for (int i = 0; i < 4; ++i)
    if (!std::isfinite(v.data[i]))
      throw std::invalid_argument("Tween value must be finite");
  if (v.kind == ValueKind::Quaternion && glm::length(v.data) < 1e-8)
    throw std::invalid_argument("Tween quaternion must be nonzero");
}
Value Mix(Value a, Value b, double t) {
  if (a.kind != b.kind)
    throw std::invalid_argument("Tween getter and end value types differ");
  Value v = b;
  if (a.kind == ValueKind::Quaternion) {
    auto q = glm::normalize(glm::slerp(glm::normalize(a.As<Quaternion>()),
                                       glm::normalize(b.As<Quaternion>()),
                                       float(t)));
    v = Value(q);
  } else
    v.data = glm::mix(a.data, b.data, t);
  return v;
}
struct Child {
  uint64_t id;
  double at;
};
struct Record {
  uint64_t id = 0, parent = 0;
  bool sequence = false, playing = true, started = false, captured = false,
       complete = false, killed = false, autoKill = true, independent = false,
       managed = false;
  double duration = 0, delay = 0, time = 0, lastAppend = 0;
  int loops = 1;
  LoopType loop = LoopType::Restart;
  Ease ease = Ease::OutQuad;
  UpdateType update = UpdateType::Normal;
  Value start, end;
  std::function<Value()> get;
  std::function<void(Value)> set;
  std::array<std::function<void()>, 5> callbacks;
  std::vector<Child> children;
  Entity target = nullptr, link = nullptr;
  uint64_t component = 0;
  std::string channel;
  LinkBehaviour behaviour = LinkBehaviour::KillOnDestroy;
  std::function<bool()> valid;
  double Length() const {
    return loops < 0 ? INFINITY : delay + duration * loops;
  }
};
struct Pending {
  uint64_t id;
  Event event;
};
std::atomic<uint64_t> nextId{1};
} // namespace
struct State {
  Scene *owner = nullptr;
  std::map<uint64_t, std::unique_ptr<Record>> records;
  std::vector<std::unique_ptr<Record>> pool;
  std::vector<uint64_t> roots;
  std::vector<Pending> events;
  bool updating = false, delivering = false;
  Record *Get(uint64_t id) const {
    auto it = records.find(id);
    return it == records.end() ? nullptr : it->second.get();
  }
  Record &Require(uint64_t id) const {
    auto *r = Get(id);
    if (!r || r->killed)
      throw std::runtime_error("Tween is no longer active");
    return *r;
  }
  Record &New() {
    auto p = pool.empty() ? std::make_unique<Record>() : std::move(pool.back());
    if (!pool.empty())
      pool.pop_back();
    p->id = nextId++;
    auto id = p->id;
    records.emplace(id, std::move(p));
    return *records.at(id);
  }
  uint64_t Root(uint64_t id) const {
    while (auto *r = Get(id)) {
      if (!r->parent)
        break;
      id = r->parent;
    }
    return id;
  }
  void Emit(Record &r, Event e) {
    if (r.callbacks[int(e)])
      events.push_back({r.id, e});
  }
  void Kill(uint64_t id, bool notify = true, bool root = true) {
    auto *r = Get(root ? Root(id) : id);
    if (!r || r->killed)
      return;
    r->killed = true;
    r->playing = false;
    for (auto c : r->children)
      Kill(c.id, notify, false);
    if (notify)
      Emit(*r, Event::Kill);
  }
  void Fail(uint64_t id, const std::exception &e) {
    Debug::Warning("Tween %llu: %s", (unsigned long long)id, e.what());
    Kill(id);
  }
  bool Alive(Record &r) {
    if (r.valid && !r.valid()) {
      Kill(r.id);
      return false;
    }
    if (r.target.GetHandle() != entt::null && !r.target.IsValid()) {
      Kill(r.id);
      return false;
    }
    if (r.link.GetHandle() != entt::null && !r.link.IsValid()) {
      Kill(r.id);
      return false;
    }
    return !r.killed;
  }
  bool Disabled(const Record &r) const {
    if (r.target.IsValid() && !r.target.Active())
      return true;
    return r.behaviour == LinkBehaviour::PauseOnDisableResumeOnEnable &&
           r.link.IsValid() && !r.link.Active();
  }
  bool Ready(Record &r) {
    if (!Alive(r) || Disabled(r))
      return false;
    for (auto child : r.children) {
      auto *record = Get(child.id);
      if (!record || !Ready(*record))
        return false;
    }
    return !r.killed;
  }
  void Activate(Record &r) {
    if ((!r.started || r.complete) && !r.channel.empty()) {
      for (auto &[id, other] : records)
        if (id != r.id && other->captured && !other->complete &&
            !other->killed && other->channel == r.channel &&
            Root(id) != Root(r.id))
          Kill(id);
    }
    if (r.captured)
      return;
    if (!r.sequence) {
      r.start = r.get();
      Validate(r.start);
      if (r.start.kind != r.end.kind)
        throw std::invalid_argument("Tween value types differ");
    }
    r.captured = true;
  }
  struct Interval {
    std::string channel;
    double start, end;
  };
  void Intervals(const Record &r, double origin, std::vector<Interval> &result,
                 int depth = 0) const {
    if (depth > 64)
      throw std::invalid_argument("Sequence nesting exceeds 64 levels");
    if (!r.channel.empty())
      result.push_back({r.channel, origin + r.delay, origin + r.Length()});
    if (r.sequence) {
      if (r.loops > 100000)
        throw std::invalid_argument("Too many nested sequence loops");
      for (int i = 0; i < r.loops; ++i)
        for (auto child : r.children)
          if (auto *record = Get(child.id))
            Intervals(*record, origin + r.delay + i * r.duration + child.at,
                      result, depth + 1);
    }
  }
  void ResetPlayback(Record &r) {
    r.started = false;
    r.complete = false;
    for (auto child : r.children)
      if (auto *record = Get(child.id))
        ResetPlayback(*record);
  }
  void Segment(Record &r, double from, double to) {
    if (!r.sequence) {
      r.set(Mix(r.start, r.end,
                EvaluateEase(r.ease, r.duration > 0 ? to / r.duration : 1)));
      return;
    }
    bool forward = to >= from;
    // Timeline order ensures a later child reads the preceding child's final
    // value.
    for (size_t i = 0; i < r.children.size(); ++i) {
      auto child = r.children[forward ? i : r.children.size() - 1 - i];
      auto *c = Get(child.id);
      if (!c || c->killed)
        continue;
      double a = from - child.at, b = to - child.at;
      if (forward && (b < -epsilon || (a > c->Length() + epsilon)))
        continue;
      if (!forward && (a < -epsilon || b > c->Length() + epsilon))
        continue;
      if (!forward && c->Length() <= epsilon && a > epsilon && b <= epsilon)
        ResetPlayback(*c);
      Sample(*c, std::clamp(a, 0.0, c->Length()),
             std::clamp(b, 0.0, c->Length()), false);
      if (r.killed)
        return;
    }
  }
  void Sample(Record &r, double before, double after, bool root) {
    if (!Alive(r))
      return;
    if (after + epsilon < r.delay)
      return;
    Activate(r);
    if (!Alive(r))
      return;
    if (!r.started) {
      r.started = true;
      Emit(r, Event::Start);
    }
    double a = std::max(0.0, before - r.delay),
           b = std::max(0.0, after - r.delay);
    const double total = r.loops < 0 ? INFINITY : r.duration * r.loops;
    a = std::min(a, total);
    b = std::min(b, total);
    if (r.duration <= epsilon) {
      if (!r.complete) {
        Segment(r, 0, 0);
        Emit(r, Event::Update);
        Emit(r, Event::StepComplete);
        r.complete = true;
        Emit(r, Event::Complete);
      }
    } else {
      bool forward = b >= a;
      double cursor = a;
      // Split at loop boundaries; no loss of time remainder on long frames.
      int guard = 0;
      do {
        if (++guard > 100000)
          throw std::runtime_error("Tween step crossed too many iterations");
        double cycle =
            forward
                ? std::floor((cursor + epsilon) / r.duration)
                : std::max(0.0, std::ceil((cursor - epsilon) / r.duration) - 1);
        if (r.loops > 0)
          cycle = std::min(cycle, double(r.loops - 1));
        double next = forward ? std::min(b, (cycle + 1) * r.duration)
                              : std::max(b, cycle * r.duration);
        double x = std::clamp(cursor - cycle * r.duration, 0.0, r.duration),
               y = std::clamp(next - cycle * r.duration, 0.0, r.duration);
        if (r.loop == LoopType::Yoyo && std::fmod(cycle, 2.0) >= 1) {
          x = r.duration - x;
          y = r.duration - y;
        }
        if (r.sequence && r.loop == LoopType::Restart && forward &&
            cursor > epsilon && std::abs(x) < epsilon)
          for (auto child : r.children)
            if (auto *record = Get(child.id))
              ResetPlayback(*record);
        Segment(r, x, y);
        if (r.killed)
          return;
        if (std::abs(next - cursor) > epsilon &&
            std::abs(next / r.duration - std::round(next / r.duration)) <
                epsilon)
          Emit(r, Event::StepComplete);
        cursor = next;
      } while (std::abs(cursor - b) > epsilon);
      Emit(r, Event::Update);
      bool done = b + epsilon >= total;
      if (done && !r.complete)
        Emit(r, Event::Complete);
      r.complete = done;
    }
    if (root && r.complete)
      r.playing = false;
  }
  void Rewind(Record &r) {
    if (r.sequence) {
      for (auto it = r.children.rbegin(); it != r.children.rend(); ++it)
        if (auto *child = Get(it->id))
          Rewind(*child);
    } else if (r.captured && Alive(r))
      r.set(r.start);
    r.time = 0;
    r.started = false;
    r.complete = false;
  }
  void Sweep() {
    for (auto it = records.begin(); it != records.end();) {
      if (!it->second->killed) {
        ++it;
        continue;
      }
      auto p = std::move(it->second);
      it = records.erase(it);
      *p = Record{};
      pool.push_back(std::move(p));
    }
  }
  void Flush() {
    if (delivering)
      return;
    delivering = true;
    for (size_t i = 0; i < events.size(); ++i) {
      auto event = events[i];
      auto *r = Get(event.id);
      if (!r || (r->killed && event.event != Event::Kill))
        continue;
      auto callback = r->callbacks[int(event.event)];
      try {
        if (callback)
          callback();
      } catch (const std::exception &error) {
        Fail(event.id, error);
      } catch (...) {
        std::runtime_error error("Unknown callback exception");
        Fail(event.id, error);
      }
    }
    events.clear();
    // Completion callbacks can restart/retain a record before auto-kill is
    // applied.
    for (auto &[id, r] : records)
      if (!r->parent && r->complete && r->autoKill && !r->killed)
        Kill(id);
    for (size_t i = 0; i < events.size(); ++i) {
      auto event = events[i];
      auto *r = Get(event.id);
      if (!r)
        continue;
      auto callback = r->callbacks[int(event.event)];
      try {
        if (callback)
          callback();
      } catch (...) {
        Debug::Warning("Tween OnKill callback failed");
      }
    }
    events.clear();
    delivering = false;
    if (!updating)
      Sweep();
  }
  void Tick(double scaled, double unscaled, UpdateType mode) {
    Seconds(scaled);
    Seconds(unscaled);
    if (updating || delivering)
      throw std::runtime_error("Recursive tween update is not allowed");
    updating = true;
    roots.clear();
    for (auto &[id, r] : records)
      if (!r->parent && !r->killed)
        roots.push_back(id);
    for (auto id : roots) {
      auto *r = Get(id);
      if (!r)
        continue;
      try {
        if (!Ready(*r) || !r->playing || r->update != mode)
          continue;
        double dt = r->independent ? unscaled : scaled;
        if (dt == 0 && r->duration > 0)
          continue;
        double before = r->time;
        r->time = std::min(r->time + dt, r->Length());
        Sample(*r, before, r->time, true);
      } catch (const std::exception &error) {
        Fail(id, error);
      } catch (...) {
        std::runtime_error error("Unknown tween getter/setter exception");
        Fail(id, error);
      }
    }
    Flush();
    updating = false;
    Sweep();
  }
};
namespace {
std::shared_ptr<State> Require(const std::weak_ptr<State> &state) {
  auto s = state.lock();
  if (!s)
    throw std::runtime_error("Tween world has expired");
  return s;
}
void Editable(Record &r) {
  if (r.started || r.parent)
    throw std::runtime_error(
        "Configure a tween before playback or sequence attachment");
}
void Independent(Record &r) {
  if (r.parent)
    throw std::runtime_error("Control the owning sequence, not its child");
}
} // namespace
double EvaluateEase(Ease ease, double t) {
  t = std::clamp(t, 0.0, 1.0);
  constexpr double pi = 3.14159265358979323846;
  switch (ease) {
  case Ease::Linear:
    return t;
  case Ease::InSine:
    return 1 - std::cos(t * pi / 2);
  case Ease::OutSine:
    return std::sin(t * pi / 2);
  case Ease::InOutSine:
    return (1 - std::cos(pi * t)) / 2;
  case Ease::InQuad:
    return t * t;
  case Ease::OutQuad:
    return 1 - (1 - t) * (1 - t);
  case Ease::InOutQuad:
    return t < .5 ? 2 * t * t : 1 - std::pow(-2 * t + 2, 2) / 2;
  case Ease::InCubic:
    return t * t * t;
  case Ease::OutCubic:
    return 1 - std::pow(1 - t, 3);
  case Ease::InOutCubic:
    return t < .5 ? 4 * t * t * t : 1 - std::pow(-2 * t + 2, 3) / 2;
  }
  throw std::invalid_argument("Unknown tween ease");
}
TweenWorld::TweenWorld(Scene *scene) : state(std::make_shared<State>()) {
  state->owner = scene;
}
TweenWorld::~TweenWorld() { Clear(); }
Scene *TweenWorld::Owner() const { return state->owner; }
TweenWorld &World(Scene &scene) { return scene.tweens; }
Tween TweenWorld::Find(uint64_t id) const { return Tween(state, id); }
void TweenWorld::Update(double dt, double unscaled) {
  state->Tick(dt, unscaled, UpdateType::Normal);
}
void TweenWorld::ManualUpdate(double dt, double unscaled) {
  state->Tick(dt, unscaled, UpdateType::Manual);
}
void TweenWorld::Clear() {
  if (bindingsInstalled)
    DisconnectShortcuts(*this);
  for (auto &[id, r] : state->records)
    state->Kill(id, false);
  state->events.clear();
  if (!state->updating && !state->delivering)
    state->Sweep();
}
void TweenWorld::ClearManaged() {
  for (auto &[id, r] : state->records)
    if (r->managed)
      state->Kill(id, false);
  if (!state->updating && !state->delivering)
    state->Sweep();
}
size_t TweenWorld::ActiveCount() const {
  size_t n = 0;
  for (auto &[id, r] : state->records)
    if (!r->killed)
      ++n;
  return n;
}
void TweenWorld::KillTarget(Entity e) {
  for (auto &[id, r] : state->records)
    if (r->target == e || r->link == e)
      state->Kill(id);
  if (!state->updating)
    state->Flush();
}
void TweenWorld::Bind(Tween t, Entity e, uint64_t component, int channel,
                      std::function<bool()> valid) {
  auto &r = state->Require(t.Id());
  r.target = e;
  r.component = component;
  r.channel = std::to_string(uint32_t(e.GetHandle())) + ":" +
              std::to_string(component) + ":" + std::to_string(channel);
  r.valid = std::move(valid);
}
void TweenWorld::Invalidate(entt::entity e, uint64_t component) {
  for (auto &[id, r] : state->records)
    if (r->component == component && r->target.GetHandle() == e)
      state->Kill(id);
}
Tween Tween::Create(TweenWorld &w, std::function<Value()> get,
                    std::function<void(Value)> set, Value end,
                    double duration) {
  Seconds(duration);
  Validate(end);
  if (!get || !set)
    throw std::invalid_argument("Tween getter/setter required");
  auto &r = w.state->New();
  r.get = std::move(get);
  r.set = std::move(set);
  r.end = end;
  r.duration = duration;
  return w.Find(r.id);
}
auto Tween::Sequence(TweenWorld &w) -> Canis::Tweening::Sequence {
  auto &r = w.state->New();
  r.sequence = true;
  r.ease = Ease::Linear;
  return Canis::Tweening::Sequence(w.Find(r.id));
}
auto Tween::Sequence(Scene &scene) -> Canis::Tweening::Sequence {
  return Sequence(World(scene));
}
void Tween::Kill(Entity e) { World(e.scene).KillTarget(e); }
bool Tween::IsActive() const {
  auto s = state.lock();
  auto *r = s ? s->Get(id) : nullptr;
  return r && !r->killed;
}
bool Tween::IsPlaying() const {
  auto s = state.lock();
  auto *r = s ? s->Get(id) : nullptr;
  return r && !r->killed && r->playing && !r->complete;
}
bool Tween::IsComplete() const {
  auto s = state.lock();
  auto *r = s ? s->Get(id) : nullptr;
  return r && !r->killed && r->complete;
}
Tween &Tween::SetEase(Ease value) {
  EvaluateEase(value, .5);
  auto &r = Require(state)->Require(id);
  Editable(r);
  if (r.sequence && value != Ease::Linear)
    throw std::invalid_argument("Sequence clocks must be linear");
  r.ease = value;
  return *this;
}
Tween &Tween::SetDelay(double v) {
  Seconds(v);
  auto &r = Require(state)->Require(id);
  Editable(r);
  r.delay = v;
  return *this;
}
Tween &Tween::SetLoops(int n, LoopType mode) {
  auto &r = Require(state)->Require(id);
  Editable(r);
  if ((n < 1 && n != -1) ||
      (mode != LoopType::Restart && mode != LoopType::Yoyo) ||
      (n == -1 && r.duration <= epsilon))
    throw std::invalid_argument("Invalid tween loops");
  r.loops = n;
  r.loop = mode;
  return *this;
}
Tween &Tween::SetAutoKill(bool v) {
  auto &r = Require(state)->Require(id);
  Independent(r);
  r.autoKill = v;
  return *this;
}
Tween &Tween::SetUpdate(UpdateType v, bool unscaled) {
  if (v != UpdateType::Normal && v != UpdateType::Manual)
    throw std::invalid_argument("Invalid tween clock");
  auto &r = Require(state)->Require(id);
  Editable(r);
  r.update = v;
  r.independent = unscaled;
  return *this;
}
Tween &Tween::SetManaged(bool v) {
  Require(state)->Require(id).managed = v;
  return *this;
}
Tween &Tween::SetLink(Entity e, LinkBehaviour v) {
  if (!e.IsValid() || (v != LinkBehaviour::KillOnDestroy &&
                       v != LinkBehaviour::PauseOnDisableResumeOnEnable))
    throw std::invalid_argument("Tween link is invalid");
  auto s = Require(state);
  if (s->owner && s->owner != &e.scene)
    throw std::invalid_argument("Tween link belongs to another scene");
  auto &r = s->Require(id);
  Independent(r);
  r.link = e;
  r.behaviour = v;
  return *this;
}
Tween &Tween::SetPropertyKey(Entity e, const std::string &key) {
  auto &r = Require(state)->Require(id);
  Editable(r);
  SetLink(e);
  r.channel = "generic:" + std::to_string(uint32_t(e.GetHandle())) + ":" + key;
  return *this;
}
Tween &Tween::On(Event event, std::function<void()> f) {
  if (int(event) < 0 || int(event) > 4)
    throw std::invalid_argument("Invalid tween event");
  Require(state)->Require(id).callbacks[int(event)] = std::move(f);
  return *this;
}
void Tween::Play() {
  auto &r = Require(state)->Require(id);
  Independent(r);
  if (!r.complete)
    r.playing = true;
}
void Tween::Pause() {
  auto &r = Require(state)->Require(id);
  Independent(r);
  r.playing = false;
}
void Tween::Restart() {
  auto s = Require(state);
  auto &r = s->Require(id);
  Independent(r);
  bool wasUpdating = s->updating;
  s->updating = true;
  try {
    s->Rewind(r);
    r.playing = true;
  } catch (const std::exception &e) {
    s->Fail(id, e);
  } catch (...) {
    std::runtime_error e("Tween rewind failed");
    s->Fail(id, e);
  }
  s->updating = wasUpdating;
  if (!wasUpdating)
    s->Flush();
}
void Tween::Kill() {
  auto s = state.lock();
  if (!s)
    return;
  s->Kill(id);
  if (!s->updating)
    s->Flush();
}
void Tween::Complete() {
  auto s = Require(state);
  auto &r = s->Require(id);
  Independent(r);
  if (r.loops < 0)
    throw std::invalid_argument("Cannot complete infinite tween");
  if (r.complete)
    return;
  bool wasUpdating = s->updating;
  s->updating = true;
  double before = r.time;
  r.time = r.Length();
  try {
    s->Sample(r, before, r.time, true);
  } catch (const std::exception &e) {
    s->Fail(id, e);
  } catch (...) {
    std::runtime_error e("Tween completion failed");
    s->Fail(id, e);
  }
  s->updating = wasUpdating;
  if (!wasUpdating)
    s->Flush();
}
Sequence &Sequence::Insert(double at, Tween child) {
  Seconds(at);
  auto s = Require(state);
  if (child.state.lock() != s)
    throw std::invalid_argument("Sequence child belongs to another world");
  auto &r = s->Require(id);
  Editable(r);
  if (!r.sequence)
    throw std::invalid_argument("Parent must be a sequence");
  auto &c = s->Require(child.Id());
  Editable(c);
  if (c.id == r.id || c.loops < 0)
    throw std::invalid_argument("Invalid sequence child");
  // A parent cannot be adopted by a descendant, even before first playback.
  for (uint64_t p = r.id; p; p = s->Require(p).parent)
    if (p == c.id)
      throw std::invalid_argument("Sequence cycle");
  std::vector<State::Interval> added, existing;
  s->Intervals(c, at, added);
  for (auto other : r.children)
    s->Intervals(s->Require(other.id), other.at, existing);
  for (const auto &left : added)
    for (const auto &right : existing)
      if (left.channel == right.channel && left.start < right.end - epsilon &&
          right.start < left.end - epsilon)
        throw std::invalid_argument(
            "Sequence children overlap on the same property");
  c.parent = r.id;
  r.children.push_back({c.id, at});
  std::stable_sort(r.children.begin(), r.children.end(),
                   [](auto a, auto b) { return a.at < b.at; });
  r.duration = std::max(r.duration, at + c.Length());
  r.lastAppend = at;
  return *this;
}
Sequence &Sequence::Append(Tween child) {
  return Insert(Require(state)->Require(id).duration, child);
}
Sequence &Sequence::Join(Tween child) {
  auto &r = Require(state)->Require(id);
  double group = r.lastAppend;
  Insert(group, child);
  r.lastAppend = group;
  return *this;
}
Sequence &Sequence::AppendInterval(double value) {
  Seconds(value);
  auto &r = Require(state)->Require(id);
  Editable(r);
  r.lastAppend = r.duration;
  r.duration += value;
  return *this;
}
Sequence &Sequence::AppendCallback(std::function<void()> callback) {
  auto s = Require(state);
  auto &c = s->New();
  c.sequence = true;
  c.callbacks[int(Event::Complete)] = std::move(callback);
  return Append(Tween(s, c.id));
}
} // namespace Canis::Tweening
