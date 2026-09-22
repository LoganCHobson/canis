#pragma once
#include <Canis/External/entt.hpp>
#include <Canis/Tweening/Tween.hpp>
namespace Canis::Tweening {
class TweenWorld {
public:
  explicit TweenWorld(Scene *scene = nullptr);
  ~TweenWorld();
  TweenWorld(const TweenWorld &) = delete;
  TweenWorld &operator=(const TweenWorld &) = delete;
  Tween Find(uint64_t id) const;
  void Update(double scaledDelta, double unscaledDelta);
  void ManualUpdate(double delta, double unscaledDelta);
  void Clear();
  void ClearManaged();
  void KillTarget(Entity);
  size_t ActiveCount() const;
  Scene *Owner() const;
  // Property adapters validate target/component identity on every sample.
  void Bind(Tween, Entity, uint64_t component, int channel,
            std::function<bool()> valid);
  void Invalidate(entt::entity, uint64_t component);
  template <class T> void Removed(entt::registry &, entt::entity e) {
    Invalidate(e, entt::type_hash<T>::value());
  }
  bool bindingsInstalled = false;

private:
  friend class Tween;
  friend class Sequence;
  std::shared_ptr<State> state;
};
} // namespace Canis::Tweening
