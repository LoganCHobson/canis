using Canis;
using Canis.Tests;
using Canis.Tweening;

// Minimal browser fixture; also run under the desktop host for parity.
public sealed class TweenWebScenario : GameSystem
{
    private float progress;
    public override void Start()
    {
        TweenApiChecks.Run();
        Tween.To(() => progress, value => progress = value, 1f, .1)
            .OnComplete(() => Log.Info("CANIS_TWEEN_WEB_PASS automatic=" + progress));
        Log.Info("CANIS_TWEEN_WEB_API_PASS");
    }
}
