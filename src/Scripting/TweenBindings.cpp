#include <Canis/App.hpp>
#include <Canis/Components.hpp>
#include <Canis/Scripting/NativeBindings.hpp>
#include <Canis/Scripting/TweenBindings.hpp>
#include <Canis/Tweening/Shortcuts.hpp>

namespace Canis::Scripting {
namespace {
using namespace Tweening;
struct Context : std::enable_shared_from_this<Context> {
  Context(App &app, std::function<Entity(uint64_t)> resolve)
      : app(app), entity(std::move(resolve)) {}
  App &app;
  std::function<Entity(uint64_t)> entity;
  std::vector<Value *> exchange;
  void Call(int registration, int action, Value *value = nullptr) {
    if (!app.scene.managedTween)
      throw std::runtime_error("Managed tween host is unavailable");
    exchange.push_back(value);
    try {
      app.scene.managedTween(registration, action);
    } catch (...) {
      exchange.pop_back();
      throw;
    }
    exchange.pop_back();
  }
  Value &Current() {
    if (exchange.empty() || !exchange.back())
      throw std::runtime_error("No managed tween value request");
    return *exchange.back();
  }
  Tween Get(uint64_t id) { return app.scene.tweens.Find(id); }
  Tween Own(Tween t, int registration) {
    t.SetManaged().OnKill([self = shared_from_this(), registration] {
      self->Call(registration, 6);
    });
    return t;
  }
};
Value Unpack(int kind, Vector4 vector, double number) {
  switch (ValueKind(kind)) {
  case ValueKind::Number:
    return Value(number);
  case ValueKind::Vector2:
    return Value(Vector2(vector));
  case ValueKind::Vector3:
    return Value(Vector3(vector));
  case ValueKind::Vector4:
    return Value(vector);
  case ValueKind::Quaternion:
    return Value(Quaternion(vector.w, vector.x, vector.y, vector.z));
  }
  throw std::invalid_argument("Unknown tween value kind");
}
} // namespace
void RegisterTweenBindings(App &app, std::function<Entity(uint64_t)> resolve) {
  auto c = std::make_shared<Context>(app, std::move(resolve));
  auto &b = NativeBindingRegistry::Get();
  const auto owner = "Canis.Scene";
  b.Method(owner, "Tween", "Property",
           [c](uint64_t entity, int property, int kind, Vector4 end,
               double number, double seconds, int registration) -> uint64_t {
             return c
                 ->Own(PropertyTween(c->entity(entity), Property(property),
                                     Unpack(kind, end, number), seconds),
                       registration)
                 .Id();
           });
  b.Method(owner, "Tween", "Value",
           [c](int kind, Vector4 end, double number, double seconds,
               int registration) -> uint64_t {
             Value target = Unpack(kind, end, number);
             return c
                 ->Own(Tween::Create(
                           c->app.scene.tweens,
                           [c, registration, target] {
                             Value v = target;
                             c->Call(registration, 0, &v);
                             return v;
                           },
                           [c, registration](Value v) {
                             c->Call(registration, 1, &v);
                           },
                           target, seconds),
                       registration)
                 .Id();
           });
  b.Method(owner, "Tween", "Sequence", [c](int registration) -> uint64_t {
    return c->Own(Tween::Sequence(c->app.scene), registration).Id();
  });
  b.Method(owner, "Tween", "ReadVector",
           [c]() -> Vector4 { return Vector4(c->Current().data); });
  b.Method(owner, "Tween", "ReadNumber",
           [c]() -> double { return c->Current().data.x; });
  b.Method(owner, "Tween", "WriteVector",
           [c](Vector4 v) { c->Current().data = glm::dvec4(v); });
  b.Method(owner, "Tween", "WriteNumber",
           [c](double v) { c->Current().data.x = v; });
  b.Method(owner, "Tween", "On", [c](uint64_t id, int event, int registration) {
    c->Get(id).On(Event(event), [c, event, registration] {
      c->Call(registration, event + 2);
    });
  });
  b.Method(owner, "Tween", "Ease",
           [c](uint64_t id, int v) { c->Get(id).SetEase(Ease(v)); });
  b.Method(owner, "Tween", "Delay",
           [c](uint64_t id, double v) { c->Get(id).SetDelay(v); });
  b.Method(owner, "Tween", "Loops", [c](uint64_t id, int n, int type) {
    c->Get(id).SetLoops(n, LoopType(type));
  });
  b.Method(owner, "Tween", "AutoKill",
           [c](uint64_t id, bool v) { c->Get(id).SetAutoKill(v); });
  b.Method(owner, "Tween", "Update",
           [c](uint64_t id, int type, bool independent) {
             c->Get(id).SetUpdate(UpdateType(type), independent);
           });
  b.Method(owner, "Tween", "Link",
           [c](uint64_t id, uint64_t entity, int behaviour) {
             c->Get(id).SetLink(c->entity(entity), LinkBehaviour(behaviour));
           });
  b.Method(owner, "Tween", "PropertyKey",
           [c](uint64_t id, uint64_t entity, std::string key) {
             c->Get(id).SetPropertyKey(c->entity(entity), key);
           });
  b.Method(owner, "Tween", "Control", [c](uint64_t id, int action) {
    auto t = c->Get(id);
    switch (action) {
    case 0:
      t.Play();
      break;
    case 1:
      t.Pause();
      break;
    case 2:
      t.Restart();
      break;
    case 3:
      t.Kill();
      break;
    case 4:
      t.Complete();
      break;
    default:
      throw std::invalid_argument("Invalid tween control");
    }
  });
  b.Method(owner, "Tween", "Status", [c](uint64_t id, int query) -> bool {
    auto t = c->Get(id);
    switch (query) {
    case 0:
      return t.IsActive();
    case 1:
      return t.IsPlaying();
    case 2:
      return t.IsComplete();
    default:
      throw std::invalid_argument("Invalid tween query");
    }
  });
  b.Method(owner, "Tween", "Add",
           [c](uint64_t id, uint64_t child, int action, double at) {
             Sequence s(c->Get(id));
             switch (action) {
             case 0:
               s.Append(c->Get(child));
               break;
             case 1:
               s.Join(c->Get(child));
               break;
             case 2:
               s.Insert(at, c->Get(child));
               break;
             case 3:
               s.AppendInterval(at);
               break;
             default:
               throw std::invalid_argument("Invalid sequence operation");
             }
           });
  b.Method(owner, "Tween", "KillTarget",
           [c](uint64_t id) { c->app.scene.tweens.KillTarget(c->entity(id)); });
  b.Method(owner, "Tween", "ClearManaged",
           [c] { c->app.scene.tweens.ClearManaged(); });
  b.Method(owner, "Tween", "ManualUpdate", [c](double scaled, double unscaled) {
    c->app.scene.tweens.ManualUpdate(scaled, unscaled);
  });
  b.Method(owner, "Tween", "ActiveCount",
           [c]() -> int { return int(c->app.scene.tweens.ActiveCount()); });
}
} // namespace Canis::Scripting
