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

These builds use the project as it is on disk. The editor's Release window also lets you choose the startup scene; see Release builds below.

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

## Project settings

Android player settings live in `project_settings/project.canis` and are edited in the editor under **Project Settings > Android**:

- package name (empty builds as `org.canis.<executableName>`)
- version name and version code (the code must increase with every Google Play upload)
- orientation and minimum Android API
- icons: an app icon (falls back to the project icon) and optional adaptive icon foreground and background layers with a background color

Icons are square PNG texture assets, 512px or larger. Every launcher density, the adaptive icon and a 512px `play-store-icon.png` are generated from them at build time.

Any `android*` key can be overridden for one build with a Gradle property, for example `-PandroidVersionCode=42`.

## Release builds

The editor's **Release** window has an **Android** platform that builds an APK or an App Bundle for Google Play through `scripts/build-release.py`, the same script the other platforms use.

Release builds are signed when **Project Settings > Android** names a keystore and key alias. Passwords are never saved: the Release window asks for them, or set `CANIS_KEYSTORE_PASSWORD` and `CANIS_KEY_PASSWORD` when running Gradle directly. Create a keystore once with the JDK's `keytool`:

```bash
keytool -genkeypair -v -keystore release.keystore -alias game -keyalg RSA -keysize 2048 -validity 10000
```

Keep the keystore out of version control and backed up; losing it means the app can no longer be updated.

## Notes

- The editor runtime is disabled for Android builds.
- Gameplay code is statically linked into `libmain.so` instead of being hot-loaded as a shared library.
- The Android target uses OpenGL ES 3 shader compilation, the same path as web.
- Assets are packed inside the APK. On launch the runtime mirrors them into internal storage and runs from there, so file access works as it does on desktop. The mirror is refreshed only when `canis_files.txt` changes.
- Only `arm64-v8a` is built by default. Add ABIs in `project/android/app/build.gradle`.
- C#, Steam Audio, Steam Input and OpenXR are unavailable on Android.
