#pragma once
#include <Canis/UUID.hpp>
#include <Canis/AssetHandle.hpp>
#include <Canis/Math.hpp>
#include <Canis/GpuPreference.hpp>
#include <string>
#include <vector>

namespace Canis
{
    enum ProjectSyncMode : int
    {
        PROJECT_SYNC_ADAPTIVE = -1,
        PROJECT_SYNC_OFF = 0,
        PROJECT_SYNC_VSYNC = 1,
    };

    enum ProjectWindowMode : int
    {
        PROJECT_WINDOW_WINDOWED = 0,
        PROJECT_WINDOW_BORDERLESS = 1,
        PROJECT_WINDOW_FULLSCREEN = 2,
    };

    enum ProjectOrientation : int
    {
        PROJECT_ORIENTATION_AUTO = 0,
        PROJECT_ORIENTATION_PORTRAIT = 1,
        PROJECT_ORIENTATION_LANDSCAPE = 2,
        PROJECT_ORIENTATION_SENSOR_PORTRAIT = 3,
        PROJECT_ORIENTATION_SENSOR_LANDSCAPE = 4,
    };

    enum EditorThemeMode : int
    {
        EDITOR_THEME_DARK = 0,
        EDITOR_THEME_LIGHT = 1,
    };

    // Uppercase-leading identifiers avoid C++ keywords and reserved identifiers.
    inline bool IsValidProjectTag(const std::string& name)
    {
        if (name.empty() || name == "None" || name[0] < 'A' || name[0] > 'Z' ||
            name.find("__") != std::string::npos)
            return false;
        for (const char c : name)
            if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                  (c >= '0' && c <= '9') || c == '_'))
                return false;
        return true;
    }

    struct ProjectConfig
    {
        std::string inputAsset = "project_settings/input.canis";
        // Human-facing title and build-target identity are deliberately
        // separate: game names may contain spaces while executable names must
        // remain suitable for the host platform and build system.
        std::string gameName = "Canis Game";
        std::string executableName = "c-engine";
        std::vector<std::string> tags = {};
        //bool fullscreen = false;
        //bool borderless = false;
        //bool resizeable = false;
        //int width = 1280;
        //int heigth = 720;
        bool useFrameLimit = false;
        int frameLimit = 120.0f;
        int frameLimitEditor = 120.0f;
        bool overrideSeed = false;
        unsigned int seed = 0;
        float volume = 1.0f;
        float musicVolume = 1.0f;
        float sfxVolume = 1.0f;
        bool mute = false;
        //bool log = true;
        //bool logToFile = false;
        bool editor = true;
        int syncMode = PROJECT_SYNC_OFF;
        int gpuPreference = PROJECT_GPU_AUTOMATIC;
        UUID iconUUID = UUID(0);
        SceneAssetHandle launchScene = {};
        std::string launchExecutablePath = "";
        std::string launchWorkingDirectory = "";
        std::string launchArguments = "";
        int editorWindowWidth = 512;
        int editorWindowHeight = 512;
        int targetGameWidth = 512;
        int targetGameHeight = 512;
        int windowMode = PROJECT_WINDOW_WINDOWED;
        bool windowResizable = true;
        bool windowStartMaximized = false;

        // Android player settings, read by project/android/app/build.gradle.
        // An empty package name builds as org.canis.<executableName>.
        std::string androidPackageName = "";
        std::string androidVersionName = "1.0";
        int androidVersionCode = 1;
        int androidOrientation = PROJECT_ORIENTATION_AUTO;
        int androidMinSdk = 21;
        // A zero icon falls back to iconUUID. The adaptive layers are optional
        // and used on Android 8+; without a background image the color is used.
        UUID androidIconUUID = UUID(0);
        UUID androidIconForegroundUUID = UUID(0);
        UUID androidIconBackgroundUUID = UUID(0);
        std::string androidIconBackgroundColor = "#FFFFFF";
        // Release signing. Passwords are never stored; the build reads them
        // from CANIS_KEYSTORE_PASSWORD and CANIS_KEY_PASSWORD.
        std::string androidKeystorePath = "";
        std::string androidKeyAlias = "";
        // https links the app opens directly (Android App Links), e.g. host
        // "example.github.io" and path "/game/daily". Empty disables them.
        std::string androidLinkHost = "";
        std::string androidLinkPath = "";
    };

    struct EditorSceneCameraConfig
    {
        SceneAssetHandle scene = {};
        int sceneCameraMode = 0;
        Vector3 sceneCamera3DPosition = Vector3(0.0f, 2.0f, 8.0f);
        float sceneCamera3DYaw = -90.0f;
        float sceneCamera3DPitch = -12.0f;
        float sceneCamera3DFovDegrees = 60.0f;
        Vector2 sceneCamera2DPosition = Vector2(0.0f);
        float sceneCamera2DScale = 1.0f;
    };

    struct EditorConfig
    {
        SceneAssetHandle lastEditorScene = {};
        ShaderGraphAssetHandle lastShaderGraph = {};
        AnimationClipAssetHandle lastAnimationClip = {};
        int theme = EDITOR_THEME_DARK;
        std::string fontPath = "";
        float fontScale = 1.0f;
        bool reloadBuildAutoCloseOnSuccess = false;
        bool showBlockoutGrid = true;
        bool gridSnappingEnabled = true;
        float translationSnap = 0.25f;
        float rotationSnapDegrees = 15.0f;
        float scaleSnap = 0.25f;
        int sceneCameraMode = 0;
        Vector3 sceneCamera3DPosition = Vector3(0.0f, 2.0f, 8.0f);
        float sceneCamera3DYaw = -90.0f;
        float sceneCamera3DPitch = -12.0f;
        float sceneCamera3DFovDegrees = 60.0f;
        Vector2 sceneCamera2DPosition = Vector2(0.0f);
        float sceneCamera2DScale = 1.0f;
        std::vector<EditorSceneCameraConfig> sceneCameras = {};
    };

    ProjectConfig& GetProjectConfig();
    bool IsValidProjectExecutableName(const std::string &_name);
    bool IsValidAndroidPackageName(const std::string &_name);
    bool IsValidHexColor(const std::string &_color);
    EditorConfig& GetEditorConfig();
    bool IsEditorRuntimeEnabled();
    void SetEditorRuntimeEnabled(bool _enabled);
    extern bool SaveProjectConfig();
    extern bool SaveEditorConfig();

    extern int Init();

} // end of Canis namespace
