namespace Canis;

/// <summary>Persistent per-user preferences shared with C++. Main thread only.</summary>
public static class PlayerPrefs
{
    public static bool HasKey(string key) => NativeBridge.Call<bool>("PlayerPrefs.HasKey", key);
    public static string GetString(string key, string fallback = "") => NativeBridge.Call<string>("PlayerPrefs.GetString", key, fallback);
    public static void SetString(string key, string value) => NativeBridge.Call("PlayerPrefs.SetString", key, value);
    public static int GetInt(string key, int fallback = 0) => NativeBridge.Call<int>("PlayerPrefs.GetInt", key, fallback);
    public static void SetInt(string key, int value) => NativeBridge.Call("PlayerPrefs.SetInt", key, value);
    public static float GetFloat(string key, float fallback = 0) => NativeBridge.Call<float>("PlayerPrefs.GetFloat", key, fallback);
    public static void SetFloat(string key, float value) => NativeBridge.Call("PlayerPrefs.SetFloat", key, value);
    public static bool GetBool(string key, bool fallback = false) => NativeBridge.Call<bool>("PlayerPrefs.GetBool", key, fallback);
    public static void SetBool(string key, bool value) => NativeBridge.Call("PlayerPrefs.SetBool", key, value);
    public static void DeleteKey(string key) => NativeBridge.Call("PlayerPrefs.DeleteKey", key);
    public static void DeleteAll() => NativeBridge.Call("PlayerPrefs.DeleteAll");
    public static void Save() => NativeBridge.Call("PlayerPrefs.Save");
}
