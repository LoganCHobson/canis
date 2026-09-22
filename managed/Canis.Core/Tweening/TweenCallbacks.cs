namespace Canis.Tweening;

// Native requests contain only integer registration/action codes. No delegate
// pointer crosses the bridge, including on WebAssembly. Clear before ALC unload.
internal static class TweenCallbacks
{
    private sealed class Entry(Action? read, Action? write)
    {
        internal readonly Action? Read = read, Write = write;
        internal readonly Action?[] Events = new Action?[5];
    }
    private static readonly Dictionary<int, Entry> entries = [];
    private static int serial;
    internal static int Add(Action? read = null, Action? write = null)
    {
        int id = checked(++serial);
        entries.Add(id, new(read, write));
        return id;
    }
    internal static void Remove(int id) => entries.Remove(id);
    internal static void Set(int id, int action, Action callback) => entries[id].Events[action] = callback;
    internal static void Invoke(int id, int action)
    {
        if (!entries.TryGetValue(id, out var entry)) return;
        try
        {
            if (action == 0) entry.Read?.Invoke();
            else if (action == 1) entry.Write?.Invoke();
            else entry.Events[action - 2]?.Invoke();
        }
        catch (Exception error) { Log.ReportException(error, $"Tween callback {id}"); throw; }
        finally { if (action == 6) entries.Remove(id); }
    }
    internal static void Clear()
    {
        if (entries.Count == 0) return;
        try { NativeBridge.Call("Tween.ClearManaged"); }
        finally { entries.Clear(); }
    }
}
