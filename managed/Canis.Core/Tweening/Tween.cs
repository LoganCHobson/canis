using System.Numerics;

namespace Canis.Tweening;

public enum Ease { Linear, InSine, OutSine, InOutSine, InQuad, OutQuad, InOutQuad, InCubic, OutCubic, InOutCubic }
public enum LoopType { Restart, Yoyo }
public enum UpdateType { Normal, Manual }
public enum LinkBehaviour { KillOnDestroy, PauseOnDisableResumeOnEnable }

/// <summary>Non-owning handle to a scene's native tween scheduler. Main thread only.</summary>
public class Tween
{
    internal readonly ulong Handle;
    private readonly int registration;
    internal Tween(ulong handle, int registration) { Handle = handle; this.registration = registration; }
    public bool IsActive => NativeBridge.Call<bool>("Tween.Status", Handle, 0);
    public bool IsPlaying => NativeBridge.Call<bool>("Tween.Status", Handle, 1);
    public bool IsComplete => NativeBridge.Call<bool>("Tween.Status", Handle, 2);
    public Tween SetEase(Ease ease) { NativeBridge.Call("Tween.Ease", Handle, (int)ease); return this; }
    public Tween SetDelay(double seconds) { NativeBridge.Call("Tween.Delay", Handle, seconds); return this; }
    public Tween SetLoops(int count, LoopType type = LoopType.Restart) { NativeBridge.Call("Tween.Loops", Handle, count, (int)type); return this; }
    public Tween SetAutoKill(bool enabled) { NativeBridge.Call("Tween.AutoKill", Handle, enabled); return this; }
    public Tween SetUpdate(UpdateType type, bool independentUpdate = false) { NativeBridge.Call("Tween.Update", Handle, (int)type, independentUpdate); return this; }
    public Tween SetLink(Entity owner, LinkBehaviour behaviour = LinkBehaviour.KillOnDestroy) { NativeBridge.Call("Tween.Link", Handle, owner.Handle, (int)behaviour); return this; }
    public Tween SetPropertyKey(Entity owner, string key) { NativeBridge.Call("Tween.PropertyKey", Handle, owner.Handle, key); return this; }
    private Tween On(int kind, Action action)
    {
        ArgumentNullException.ThrowIfNull(action);
        NativeBridge.Call("Tween.On", Handle, kind, registration);
        TweenCallbacks.Set(registration, kind, action);
        return this;
    }
    public Tween OnStart(Action action) => On(0, action);
    public Tween OnUpdate(Action action) => On(1, action);
    public Tween OnStepComplete(Action action) => On(2, action);
    public Tween OnComplete(Action action) => On(3, action);
    public Tween OnKill(Action action) => On(4, action);
    public void Play() => NativeBridge.Call("Tween.Control", Handle, 0);
    public void Pause() => NativeBridge.Call("Tween.Control", Handle, 1);
    public void Restart() => NativeBridge.Call("Tween.Control", Handle, 2);
    public void Kill() => NativeBridge.Call("Tween.Control", Handle, 3);
    public void Complete() => NativeBridge.Call("Tween.Control", Handle, 4);
    public static void Kill(Entity target) => NativeBridge.Call("Tween.KillTarget", target.Handle);
    public static void ManualUpdate(double delta, double unscaledDelta) => NativeBridge.Call("Tween.ManualUpdate", delta, unscaledDelta);
    public static int ActiveCount => NativeBridge.Call<int>("Tween.ActiveCount");

    public static Tween To<T>(Func<T> getter, Action<T> setter, T end, double seconds)
    {
        ArgumentNullException.ThrowIfNull(getter); ArgumentNullException.ThrowIfNull(setter);
        var value = Pack(end);
        int id = TweenCallbacks.Add(() => Write(getter()), () => setter(Read<T>()));
        try { return new(NativeBridge.Call<ulong>("Tween.Value", value.kind, value.vector, value.number, seconds, id), id); }
        catch { TweenCallbacks.Remove(id); throw; }
    }
    public static Sequence Sequence()
    {
        int id = TweenCallbacks.Add();
        try { return new(NativeBridge.Call<ulong>("Tween.Sequence", id), id); }
        catch { TweenCallbacks.Remove(id); throw; }
    }
    internal static Tween Property<T>(NativeComponent target, int property, T end, double seconds)
    {
        target.Validate(); var value = Pack(end);
        int id = TweenCallbacks.Add();
        try { return new(NativeBridge.Call<ulong>("Tween.Property", target.Entity.Handle, property, value.kind, value.vector, value.number, seconds, id), id); }
        catch { TweenCallbacks.Remove(id); throw; }
    }
    private static (int kind, Vector4 vector, double number) Pack<T>(T value) => value switch
    {
        float v => (0, default, v), double v => (0, default, v),
        Vector2 v => (1, new(v, 0, 0), 0), Vector3 v => (2, new(v, 0), 0),
        Vector4 v => (3, v, 0), Quaternion v => (4, new(v.X, v.Y, v.Z, v.W), 0),
        _ => throw new ArgumentException("Tween values must be float, double, Vector2/3/4, or Quaternion.")
    };
    private static void Write<T>(T value)
    {
        var packed = Pack(value);
        if (packed.kind == 0) NativeBridge.Call("Tween.WriteNumber", packed.number);
        else NativeBridge.Call("Tween.WriteVector", packed.vector);
    }
    private static T Read<T>()
    {
        if (typeof(T) == typeof(float)) return (T)(object)(float)NativeBridge.Call<double>("Tween.ReadNumber");
        if (typeof(T) == typeof(double)) return (T)(object)NativeBridge.Call<double>("Tween.ReadNumber");
        var v = NativeBridge.Call<Vector4>("Tween.ReadVector");
        if (typeof(T) == typeof(Vector2)) return (T)(object)new Vector2(v.X, v.Y);
        if (typeof(T) == typeof(Vector3)) return (T)(object)new Vector3(v.X, v.Y, v.Z);
        if (typeof(T) == typeof(Quaternion)) return (T)(object)new Quaternion(v.X, v.Y, v.Z, v.W);
        return (T)(object)v;
    }
}

public sealed class Sequence : Tween
{
    internal Sequence(ulong handle, int registration) : base(handle, registration) { }
    public Sequence Append(Tween child) { NativeBridge.Call("Tween.Add", Handle, child.Handle, 0, 0d); return this; }
    public Sequence Join(Tween child) { NativeBridge.Call("Tween.Add", Handle, child.Handle, 1, 0d); return this; }
    public Sequence Insert(double at, Tween child) { NativeBridge.Call("Tween.Add", Handle, child.Handle, 2, at); return this; }
    public Sequence AppendInterval(double seconds) { NativeBridge.Call("Tween.Add", Handle, 0UL, 3, seconds); return this; }
    public Sequence AppendCallback(Action action) => Append(Tween.To(() => 0f, _ => { }, 0f, 0).OnComplete(action));
    public new Sequence SetEase(Ease value) { base.SetEase(value); return this; }
    public new Sequence SetDelay(double value) { base.SetDelay(value); return this; }
    public new Sequence SetLoops(int count, LoopType type = LoopType.Restart) { base.SetLoops(count, type); return this; }
    public new Sequence SetAutoKill(bool value) { base.SetAutoKill(value); return this; }
    public new Sequence SetUpdate(UpdateType type, bool independentUpdate = false) { base.SetUpdate(type, independentUpdate); return this; }
    public new Sequence SetLink(Entity target, LinkBehaviour behaviour = LinkBehaviour.KillOnDestroy) { base.SetLink(target, behaviour); return this; }
    public new Sequence OnStart(Action action) { base.OnStart(action); return this; }
    public new Sequence OnUpdate(Action action) { base.OnUpdate(action); return this; }
    public new Sequence OnStepComplete(Action action) { base.OnStepComplete(action); return this; }
    public new Sequence OnComplete(Action action) { base.OnComplete(action); return this; }
    public new Sequence OnKill(Action action) { base.OnKill(action); return this; }
}
