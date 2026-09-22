#include <Canis/App.hpp>
#include <Canis/Components.hpp>
#include <Canis/Editor.hpp>
#include <Canis/Tweening/Shortcuts.hpp>
#include <Canis/Tweening/TweenWorld.hpp>
#include <chrono>
#include <cmath>
#include <iostream>
#include <vector>
using namespace Canis;
using namespace Canis::Tweening;
static int checks;
void Check(bool value, const char *text) {
  ++checks;
  if (!value)
    throw std::runtime_error(text);
}
void Near(double a, double b, const char *text) {
  Check(std::abs(a - b) < 1e-5, text);
}
template <class F> void Reject(F f) {
  bool rejected = false;
  try {
    f();
  } catch (const std::exception &) {
    rejected = true;
  }
  Check(rejected, "Expected tween validation failure");
}
int main() {
  try {
    TweenWorld w;
    auto number = [&](double &value, double end, double seconds) {
      return Tween::To(
          w, [&value] { return value; }, [&value](double v) { value = v; }, end,
          seconds);
    };
    for (int hz : {30, 60, 144}) {
      double value = 0;
      auto t = number(value, 10, 1).SetEase(Ease::Linear).SetAutoKill(false);
      for (int i = 0; i < hz; ++i)
        w.Update(1.0 / hz, 1.0 / hz);
      Near(value, 10, "Frame-rate-independent endpoint");
      Check(t.IsComplete(), "Completion status");
      t.Kill();
    }
    {
      double v = 0;
      auto t = number(v, 10, 1).SetEase(Ease::Linear).SetDelay(.5);
      w.Update(.25, .25);
      v = 4;
      w.Update(.5, .5);
      Near(v, 5.5, "Delayed capture uses activation value");
      t.Kill();
    }
    {
      double v = 0;
      auto t = number(v, 10, 1)
                   .SetEase(Ease::Linear)
                   .SetLoops(2, LoopType::Yoyo)
                   .SetAutoKill(false);
      w.Update(1.25, 1.25);
      Near(v, 7.5, "Large delta across Yoyo boundary");
      w.Update(.75, .75);
      Near(v, 0, "Yoyo terminal endpoint");
      t.Restart();
      w.Update(.5, .5);
      Near(v, 5, "Restart retained range");
      t.Kill();
    }
    {
      double v = 0;
      auto t = number(v, 1, 1).SetEase(Ease::Linear);
      t.Pause();
      w.Update(.5, .5);
      Near(v, 0, "Pause does not capture or advance");
      t.Play();
      w.Update(.5, .5);
      Near(v, .5, "Play resumes");
      t.Kill();
    }
    {
      double a = 0, b = 0, c = 0;
      auto x = number(a, 1, 1);
      auto y = number(b, 1, 1).SetUpdate(UpdateType::Normal, true);
      auto z = number(c, 1, 1).SetUpdate(UpdateType::Manual);
      w.Update(0, .5);
      Near(a, 0, "Scaled clock paused");
      Near(b, .75, "Unscaled clock progresses");
      Near(c, 0, "Manual excluded from automatic update");
      w.ManualUpdate(.5, .5);
      Near(c, .75, "Manual step");
      w.Clear();
    }
    {
      double v = 0;
      std::vector<int> order;
      auto t = number(v, 5, 0)
                   .OnStart([&] { order.push_back(0); })
                   .OnUpdate([&] {
                     Near(v, 5, "Values precede callbacks");
                     order.push_back(1);
                   })
                   .OnStepComplete([&] { order.push_back(2); })
                   .OnComplete([&] { order.push_back(3); })
                   .OnKill([&] { order.push_back(4); });
      Check(order.empty(), "No synchronous creation callbacks");
      w.Update(0, 0);
      Check(order == std::vector<int>({0, 1, 2, 3, 4}), "Callback order");
      Check(!t.IsActive(), "Auto-kill invalidates handle");
      auto replacement = number(v, 6, 1);
      Check(!t.IsActive() && replacement.Id() != t.Id(),
            "Recycled slot never revives stale handle");
      w.Clear();
    }
    {
      double v = 0;
      Tween created;
      auto t = number(v, 1, .1).OnComplete([&] { created = number(v, 2, 1); });
      w.Update(.2, .2);
      Near(v, 1, "Callback-created tween waits for next frame");
      w.Update(.5, .5);
      Near(v, 1.75, "New tween progresses next frame");
      w.Clear();
    }
    {
      double v = 0;
      int complete = 0, kill = 0;
      Tween t = number(v, 1, 1);
      t.OnStart([&] { t.Kill(); }).OnComplete([&] { ++complete; }).OnKill([&] {
        ++kill;
      });
      w.Update(1, 1);
      Check(complete == 0 && kill == 1,
            "Kill suppresses queued completion and fires once");
    }
    {
      double a = 0, b = 0;
      number(a, 1, 1).OnUpdate(
          [] { throw std::runtime_error("Expected callback failure"); });
      number(b, 1, 1);
      w.Update(.5, .5);
      w.Update(.5, .5);
      Near(b, 1, "Callback failure isolated");
      Check(w.ActiveCount() == 0, "Faulted tween retired");
    }
    {
      double v = 0;
      auto s = Tween::Sequence(w);
      s.Append(number(v, 10, 1).SetEase(Ease::Linear))
          .Append(number(v, 20, 1).SetEase(Ease::Linear));
      w.Update(1.5, 1.5);
      Near(v, 15, "Chained child captures preceding end on long frame");
      w.Update(.5, .5);
      Near(v, 20, "Sequence final value");
      Check(w.ActiveCount() == 0, "Sequence releases children");
    }
    {
      double a = 0, b = 0;
      int callback = 0;
      auto s = Tween::Sequence(w);
      s.Append(number(a, 10, 1).SetEase(Ease::Linear))
          .Join(number(b, 20, 2).SetEase(Ease::Linear))
          .AppendInterval(.5)
          .AppendCallback([&] { ++callback; });
      w.Update(1, 1);
      Near(a, 10, "Joined first complete");
      Near(b, 10, "Joined second halfway");
      w.Update(1.5, 1.5);
      Near(b, 20, "Join determines sequence duration");
      Check(callback == 1, "Sequence callback after interval");
    }
    {
      double v = 0;
      auto s = Tween::Sequence(w);
      s.Append(number(v, 10, 1).SetEase(Ease::Linear))
          .Append(number(v, 20, 1).SetEase(Ease::Linear));
      s.SetLoops(2, LoopType::Yoyo);
      w.Update(3.5, 3.5);
      Near(v, 5, "Sequence reverses child order");
      w.Update(.5, .5);
      Near(v, 0, "Sequence Yoyo returns to origin");
    }
    {
      double v = 0;
      auto a = Tween::Sequence(w);
      auto b = Tween::Sequence(w);
      b.Append(number(v, 1, 1));
      a.Append(b);
      Reject([&] { b.Append(a); });
      a.Kill();
    }
    {
      double v = 0;
      Tween self;
      self = Tween::To(
          w, [&] { return v; },
          [&](double x) {
            v = x;
            self.Kill();
          },
          1.0, 1);
      self.Complete();
      Check(!self.IsActive(), "Setter can kill during synchronous Complete");
    }
    {
      double v = 0;
      int starts = 0;
      auto s = Tween::Sequence(w);
      s.Append(number(v, 10, 1).SetEase(Ease::Linear).OnStart([&] {
         ++starts;
       })).Append(number(v, 20, 1).SetEase(Ease::Linear));
      s.SetAutoKill(false);
      w.Update(2, 2);
      s.Restart();
      Near(v, 0, "Sequence restart rewinds shared property in reverse order");
      w.Update(.5, .5);
      Near(v, 5, "Restarted sequence samples initial range");
      Check(starts == 2, "Restarted child receives OnStart");
      s.Kill();
    }
    {
      int calls = 0;
      auto sequence = Tween::Sequence(w);
      sequence.AppendInterval(.25)
          .AppendCallback([&] { ++calls; })
          .AppendInterval(.25);
      sequence.SetLoops(3);
      w.Update(1.5, 1.5);
      Check(calls == 3, "Sequence callback repeats once per loop");
    }
    {
      double a = 0, b = 0;
      auto x = number(a, 1, 1).SetManaged();
      auto y = number(b, 1, 1);
      w.ClearManaged();
      Check(!x.IsActive() && y.IsActive(),
            "Managed reload does not cancel native tweens");
      w.Clear();
    }
    {
      double v = 0;
      Reject([&] { number(v, 1, -1); });
      Reject([&] { number(v, NAN, 1); });
      auto t = number(v, 1, 0);
      Reject([&] { t.SetLoops(-1); });
      t.Kill();
    }
    {
      App app;
      Editor editor;
      app.RegisterDefaults(editor);
      app.scene.app = &app;
      auto parent = app.scene.CreateEntity("Parent");
      auto &pt = *parent.AddComponent<Transform>();
      pt.position = {10, 2, 3};
      pt.rotation = glm::angleAxis(.7f, Vector3(0, 1, 0));
      pt.scale = Vector3(2);
      auto e = app.scene.CreateEntity("Door");
      auto &t = *e.AddComponent<Transform>();
      t.SetParent(&parent);
      t.position = Vector3(0);
      t.rotation = Quaternion(1, 0, 0, 0);
      auto move = TweenMove(e, Vector3(14, 2, 3), 1).SetEase(Ease::Linear);
      app.scene.tweens.Update(.5, .5);
      Near(t.GetGlobalPosition().x, 12, "World movement under parent");
      move.Kill();
      auto q = glm::angleAxis(1.5f, Vector3(0, 1, 0));
      auto open = TweenLocalRotateQuaternion(e, q, 1).SetEase(Ease::Linear);
      app.scene.tweens.Update(.5, .5);
      Near(glm::length(t.rotation), 1, "Normalized quaternion");
      Near(glm::angle(t.rotation), .75, "Local door halfway");
      auto close = TweenLocalRotateQuaternion(e, Quaternion(1, 0, 0, 0), 1)
                       .SetEase(Ease::Linear);
      app.scene.tweens.Update(.1, .1);
      Check(!open.IsActive() && close.IsActive(),
            "Same-channel rotation replacement");
      app.scene.tweens.Update(.9, .9);
      Near(glm::angle(t.rotation), 0, "Door reversed closed");
      auto retained = TweenLocalMove(e, Vector3(1), .1).SetAutoKill(false);
      app.scene.tweens.Update(.1, .1);
      auto competing = TweenLocalMove(e, Vector3(3), 1);
      app.scene.tweens.Update(.1, .1);
      retained.Restart();
      app.scene.tweens.Update(.05, .05);
      Check(!competing.IsActive(), "Restart reacquires its property channel");
      retained.Kill();
      auto inactiveSequence = Tween::Sequence(app.scene);
      inactiveSequence.Append(
          TweenLocalMove(e, Vector3(5), 1).SetEase(Ease::Linear));
      e.SetActive(false);
      auto position = e.GetComponent<Transform>().position;
      app.scene.tweens.Update(.5, .5);
      Check(e.GetComponent<Transform>().position == position,
            "Inactive child pauses owning sequence");
      e.SetActive(true);
      app.scene.tweens.Update(.5, .5);
      Check(inactiveSequence.IsActive(),
            "Disabled time is not consumed by sequence");
      inactiveSequence.Kill();
      auto nested = Tween::Sequence(app.scene);
      nested.Append(TweenLocalMove(e, Vector3(1), 1));
      auto containing = Tween::Sequence(app.scene);
      containing.Append(nested);
      auto overlapping = TweenLocalMove(e, Vector3(2), 1);
      Reject([&] { containing.Join(overlapping); });
      containing.Kill();
      overlapping.Kill();
      auto removed = TweenLocalMove(e, Vector3(5), 1);
      e.RemoveComponent<Transform>();
      e.AddComponent<Transform>();
      app.scene.tweens.Update(.5, .5);
      Check(!removed.IsActive(), "Component remove/re-add invalidates binding");
      Near(e.GetComponent<Transform>().position.x, 0,
           "Replacement component not modified");
      double value = 0;
      auto linked = Tween::To(
                        app.scene, [&] { return value; },
                        [&](double x) { value = x; }, 1.0, 1)
                        .SetLink(e);
      e.Destroy();
      app.scene.tweens.Update(.5, .5);
      Check(!linked.IsActive(), "Destroyed generic link cancels");
      Near(value, 0, "Destroyed owner not accessed");
      for (int i = 0; i < 1000; ++i) {
        auto node = app.scene.CreateEntity("Tween benchmark");
        node.AddComponent<Transform>();
        TweenLocalMove(node, Vector3(10), 100);
      }
      auto start = std::chrono::steady_clock::now();
      for (int i = 0; i < 120; ++i)
        app.scene.tweens.Update(1.0 / 60, 1.0 / 60);
      std::cout << "1000 native property tweens: "
                << std::chrono::duration<double, std::milli>(
                       std::chrono::steady_clock::now() - start)
                           .count() /
                       120
                << " ms/update\n";
      app.scene.Unload();
      Check(app.scene.tweens.ActiveCount() == 0,
            "Scene unload clears scheduler");
    }
    {
      std::vector<double> values(1000);
      for (auto &v : values)
        number(v, 1, 100);
      auto start = std::chrono::steady_clock::now();
      for (int i = 0; i < 240; ++i)
        w.Update(1.0 / 60, 1.0 / 60);
      auto elapsed = std::chrono::duration<double, std::milli>(
                         std::chrono::steady_clock::now() - start)
                         .count();
      std::cout << "1000 native value tweens: " << elapsed / 240
                << " ms/update\n";
      w.Clear();
    }
    std::cout << "PASS: " << checks << " native tween checks\n";
    return 0;
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
}
