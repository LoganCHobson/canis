#pragma once

namespace Canis
{
    // True when the OS is set to a dark appearance (Android, Windows,
    // macOS). False when it is light or unknown.
    bool IsSystemDarkTheme();

    // Android Java access for games that talk to Java code (ads, billing).
    // Null elsewhere. The JNIEnv belongs to the calling thread; classes from
    // the app's own code must be found with FindAndroidClass, because JNI's
    // FindClass only sees system classes on native threads.
    void *GetAndroidJNIEnv();
    void *GetAndroidActivity(); // a new local reference; delete it when done
    void *FindAndroidClass(const char *_className); // e.g. "com.example.Ads"; local reference
}
