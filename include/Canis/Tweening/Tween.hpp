#pragma once
#include <Canis/Math.hpp>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>

namespace Canis {
class Scene;
class Entity;
} // namespace Canis
namespace Canis::Tweening {
enum class Ease {
  Linear,
  InSine,
  OutSine,
  InOutSine,
  InQuad,
  OutQuad,
  InOutQuad,
  InCubic,
  OutCubic,
  InOutCubic
};
enum class LoopType { Restart, Yoyo };
enum class UpdateType { Normal, Manual };
enum class LinkBehaviour { KillOnDestroy, PauseOnDisableResumeOnEnable };
enum class Event { Start, Update, StepComplete, Complete, Kill };
enum class ValueKind { Number, Vector2, Vector3, Vector4, Quaternion };
struct Value {
  ValueKind kind = ValueKind::Number;
  glm::dvec4 data{0};
  Value() = default;
  Value(double v) : data(v, 0, 0, 0) {}
  Value(float v) : Value(double(v)) {}
  Value(Canis::Vector2 v) : kind(ValueKind::Vector2), data(v.x, v.y, 0, 0) {}
  Value(Canis::Vector3 v) : kind(ValueKind::Vector3), data(v.x, v.y, v.z, 0) {}
  Value(Canis::Vector4 v) : kind(ValueKind::Vector4), data(v) {}
  Value(Canis::Quaternion v)
      : kind(ValueKind::Quaternion), data(v.x, v.y, v.z, v.w) {}
  template <class T> T As() const {
    if constexpr (std::is_arithmetic_v<T>)
      return T(data.x);
    else if constexpr (std::is_same_v<T, Canis::Quaternion>)
      return T(data.w, data.x, data.y, data.z);
    else
      return T(data);
  }
};
double EvaluateEase(Ease ease, double time);
struct State;
class TweenWorld;
class Sequence;
class Tween {
public:
  Tween() = default;
  uint64_t Id() const { return id; }
  bool IsActive() const;
  bool IsPlaying() const;
  bool IsComplete() const;
  Tween &SetEase(Ease);
  Tween &SetDelay(double);
  Tween &SetLoops(int, LoopType = LoopType::Restart);
  Tween &SetAutoKill(bool);
  Tween &SetUpdate(UpdateType, bool independentUpdate = false);
  Tween &SetLink(Entity, LinkBehaviour = LinkBehaviour::KillOnDestroy);
  Tween &SetPropertyKey(Entity, const std::string &);
  Tween &SetManaged(bool = true);
  Tween &On(Event, std::function<void()>);
  Tween &OnStart(std::function<void()> f) {
    return On(Event::Start, std::move(f));
  }
  Tween &OnUpdate(std::function<void()> f) {
    return On(Event::Update, std::move(f));
  }
  Tween &OnStepComplete(std::function<void()> f) {
    return On(Event::StepComplete, std::move(f));
  }
  Tween &OnComplete(std::function<void()> f) {
    return On(Event::Complete, std::move(f));
  }
  Tween &OnKill(std::function<void()> f) {
    return On(Event::Kill, std::move(f));
  }
  void Play();
  void Pause();
  void Restart();
  void Kill();
  void Complete();
  static Tween Create(TweenWorld &, std::function<Value()>,
                      std::function<void(Value)>, Value, double);
  template <class G, class S, class T>
  static Tween To(TweenWorld &world, G get, S set, T end, double duration) {
    return Create(
        world, [get] { return Value(get()); },
        [set](Value v) { set(v.As<T>()); }, Value(end), duration);
  }
  template <class G, class S, class T>
  static Tween To(Scene &scene, G get, S set, T end, double duration);
  static auto Sequence(TweenWorld &) -> Canis::Tweening::Sequence;
  static auto Sequence(Scene &) -> Canis::Tweening::Sequence;
  static void Kill(Entity);

protected:
  friend class TweenWorld;
  friend class Canis::Tweening::Sequence;
  Tween(std::weak_ptr<State> state, uint64_t id)
      : state(std::move(state)), id(id) {}
  std::weak_ptr<State> state;
  uint64_t id = 0;
};
class Sequence : public Tween {
public:
  explicit Sequence(Tween tween) : Tween(std::move(tween)) {}
  Sequence &Append(Tween);
  Sequence &Join(Tween);
  Sequence &Insert(double, Tween);
  Sequence &AppendInterval(double);
  Sequence &AppendCallback(std::function<void()>);
};
TweenWorld &World(Scene &);
template <class G, class S, class T>
Tween Tween::To(Scene &scene, G get, S set, T end, double duration) {
  return To(World(scene), std::move(get), std::move(set), end, duration);
}
} // namespace Canis::Tweening
