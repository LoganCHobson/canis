# Android Export

The engine supports an editor-free Android export path using the Android NDK and SDL's Android activity.

## Requirements

- JDK 17 or newer
- Android SDK command-line tools with:
  - `platform-tools`
  - `platforms;android-35`
  - `build-tools;35.0.0`
  - `ndk;28.2.13676358`
  - `cmake;3.22.1`

Android Studio is not required. Point `ANDROID_HOME` at the SDK; the build script also looks in the default Android Studio locations.

## Quick start

```bash
./scripts/build-android.sh
```

That command will:

1. Locate the Android SDK through `ANDROID_HOME`
2. Run Gradle in `project/android`, which configures the root `CMakeLists.txt` with the NDK
3. Build `libmain.so` with the engine, SDL and gameplay code linked in
4. Package `project/assets` and `project/project_settings` into the APK

For a debug build:

```bash
./scripts/build-android.sh android-debug
```

For a Google Play bundle:

```bash
./scripts/build-android.sh android-bundle
```

On Windows, run the script from Git Bash.

## Output

The generated export lives in:

- `project/android/app/build/outputs/apk/release/` for `android-release`
- `project/android/app/build/outputs/apk/debug/` for `android-debug`
- `project/android/app/build/outputs/bundle/release/` for `android-bundle`

Install a debug build on a connected device:

```bash
adb install -r project/android/app/build/outputs/apk/debug/app-debug.apk
```

## Project identity

The launcher label comes from `gameName` in `project_settings/project.canis`. Set the package name and version in `project/android/gradle.properties`:

```properties
canisApplicationId=com.example.mygame
canisVersionCode=1
canisVersionName=1.0
```

Release builds and bundles must be signed before they can be installed or uploaded.

## Notes

- The editor runtime is disabled for Android builds.
- Gameplay code is statically linked into `libmain.so` instead of being hot-loaded as a shared library.
- The Android target uses OpenGL ES 3 shader compilation, the same path as web.
- Assets are packed inside the APK. On launch the runtime mirrors them into internal storage and runs from there, so file access works as it does on desktop. The mirror is refreshed only when `canis_files.txt` changes.
- Only `arm64-v8a` is built by default. Add ABIs in `project/android/app/build.gradle`.
- C#, Steam Audio, Steam Input and OpenXR are unavailable on Android.
