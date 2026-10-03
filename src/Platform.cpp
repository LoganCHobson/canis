#include <Canis/Platform.hpp>

#include <SDL3/SDL.h>

#if defined(__ANDROID__)
#include <jni.h>
#endif

namespace Canis
{
    bool IsSystemDarkTheme()
    {
        return SDL_GetSystemTheme() == SDL_SYSTEM_THEME_DARK;
    }

    void *GetAndroidJNIEnv()
    {
#if defined(__ANDROID__)
        return SDL_GetAndroidJNIEnv();
#else
        return nullptr;
#endif
    }

    void *GetAndroidActivity()
    {
#if defined(__ANDROID__)
        return SDL_GetAndroidActivity();
#else
        return nullptr;
#endif
    }

    void *FindAndroidClass(const char *_className)
    {
#if defined(__ANDROID__)
        // Go through the activity's class loader, which knows the app's classes.
        JNIEnv *env = static_cast<JNIEnv *>(SDL_GetAndroidJNIEnv());
        jobject activity = static_cast<jobject>(SDL_GetAndroidActivity());
        if (env == nullptr || activity == nullptr)
            return nullptr;

        jclass activityClass = env->GetObjectClass(activity);
        jmethodID getClassLoader = env->GetMethodID(activityClass, "getClassLoader", "()Ljava/lang/ClassLoader;");
        jobject loader = env->CallObjectMethod(activity, getClassLoader);
        jclass loaderClass = env->GetObjectClass(loader);
        jmethodID loadClass = env->GetMethodID(loaderClass, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");
        jstring name = env->NewStringUTF(_className);
        jobject found = env->CallObjectMethod(loader, loadClass, name);
        if (env->ExceptionCheck())
        {
            env->ExceptionClear();
            found = nullptr;
        }

        env->DeleteLocalRef(name);
        env->DeleteLocalRef(loaderClass);
        env->DeleteLocalRef(loader);
        env->DeleteLocalRef(activityClass);
        env->DeleteLocalRef(activity);
        return found;
#else
        (void)_className;
        return nullptr;
#endif
    }
}
