#pragma once
#include <string>

namespace Canis::PlayerPrefs
{
    // Main-thread service. Init loads from SDL's per-user directory unless a file is supplied.
    void Init(const std::string& organization, const std::string& app, const std::string& file = "");
    bool HasKey(const std::string& key);
    std::string GetString(const std::string& key, const std::string& fallback = "");
    int GetInt(const std::string& key, int fallback = 0);
    float GetFloat(const std::string& key, float fallback = 0);
    bool GetBool(const std::string& key, bool fallback = false);
    void SetString(const std::string& key, const std::string& value);
    void SetInt(const std::string& key, int value);
    void SetFloat(const std::string& key, float value);
    void SetBool(const std::string& key, bool value);
    void DeleteKey(const std::string& key);
    void DeleteAll();
    void LoadFromFile();
    void SaveToFile();
}
