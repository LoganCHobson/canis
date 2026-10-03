#include <Canis/Share.hpp>
#include <Canis/Debug.hpp>

#include <SDL3/SDL.h>

#include <cstdlib>

#if defined(__ANDROID__)
#include <jni.h>
#endif

namespace Canis
{
#if defined(__ANDROID__)
    namespace
    {
        // Intent(ACTION_SEND) with the text, wrapped in a chooser and started
        // from SDL's activity.
        bool StartShareIntent(const std::string &_text)
        {
            JNIEnv *env = static_cast<JNIEnv *>(SDL_GetAndroidJNIEnv());
            jobject activity = static_cast<jobject>(SDL_GetAndroidActivity());
            if (env == nullptr || activity == nullptr)
                return false;

            jclass intentClass = env->FindClass("android/content/Intent");
            jmethodID constructor = env->GetMethodID(intentClass, "<init>", "(Ljava/lang/String;)V");
            jmethodID setType = env->GetMethodID(intentClass, "setType", "(Ljava/lang/String;)Landroid/content/Intent;");
            jmethodID putExtra = env->GetMethodID(intentClass, "putExtra", "(Ljava/lang/String;Ljava/lang/String;)Landroid/content/Intent;");
            jmethodID createChooser = env->GetStaticMethodID(intentClass, "createChooser",
                "(Landroid/content/Intent;Ljava/lang/CharSequence;)Landroid/content/Intent;");

            jstring action = env->NewStringUTF("android.intent.action.SEND");
            jstring type = env->NewStringUTF("text/plain");
            jstring extra = env->NewStringUTF("android.intent.extra.TEXT");
            jstring text = env->NewStringUTF(_text.c_str());

            jobject intent = env->NewObject(intentClass, constructor, action);
            env->DeleteLocalRef(env->CallObjectMethod(intent, setType, type));
            env->DeleteLocalRef(env->CallObjectMethod(intent, putExtra, extra, text));
            jobject chooser = env->CallStaticObjectMethod(intentClass, createChooser, intent, nullptr);

            jclass activityClass = env->GetObjectClass(activity);
            jmethodID startActivity = env->GetMethodID(activityClass, "startActivity", "(Landroid/content/Intent;)V");
            env->CallVoidMethod(activity, startActivity, chooser);

            const bool failed = env->ExceptionCheck();
            if (failed)
                env->ExceptionClear();

            for (jobject reference : {static_cast<jobject>(intentClass), static_cast<jobject>(action), static_cast<jobject>(type),
                     static_cast<jobject>(extra), static_cast<jobject>(text), intent, chooser,
                     static_cast<jobject>(activityClass), activity})
            {
                if (reference != nullptr)
                    env->DeleteLocalRef(reference);
            }
            return !failed;
        }

        // Reads the activity's intent data and clears it so each link is
        // handled once. The activity must call setIntent() in onNewIntent()
        // for links that arrive while it is running; CanisActivity does.
        std::string TakeIntentData()
        {
            JNIEnv *env = static_cast<JNIEnv *>(SDL_GetAndroidJNIEnv());
            jobject activity = static_cast<jobject>(SDL_GetAndroidActivity());
            if (env == nullptr || activity == nullptr)
                return "";

            std::string link;
            jclass activityClass = env->GetObjectClass(activity);
            jmethodID getIntent = env->GetMethodID(activityClass, "getIntent", "()Landroid/content/Intent;");
            jobject intent = env->CallObjectMethod(activity, getIntent);
            if (intent != nullptr)
            {
                jclass intentClass = env->GetObjectClass(intent);
                jmethodID getDataString = env->GetMethodID(intentClass, "getDataString", "()Ljava/lang/String;");
                jmethodID setData = env->GetMethodID(intentClass, "setData", "(Landroid/net/Uri;)Landroid/content/Intent;");
                jstring data = static_cast<jstring>(env->CallObjectMethod(intent, getDataString));
                if (data != nullptr)
                {
                    const char *chars = env->GetStringUTFChars(data, nullptr);
                    link = chars != nullptr ? chars : "";
                    env->ReleaseStringUTFChars(data, chars);
                    env->DeleteLocalRef(data);
                    env->DeleteLocalRef(env->CallObjectMethod(intent, setData, nullptr));
                }
                env->DeleteLocalRef(intentClass);
                env->DeleteLocalRef(intent);
            }
            if (env->ExceptionCheck())
            {
                env->ExceptionClear();
                link.clear();
            }
            env->DeleteLocalRef(activityClass);
            env->DeleteLocalRef(activity);
            return link;
        }
    }
#endif

    std::string TakeOpenedLink()
    {
#if defined(__ANDROID__)
        return TakeIntentData();
#else
        static bool taken = false;
        const char *link = std::getenv("CANIS_OPEN_LINK");
        if (taken || link == nullptr)
            return "";
        taken = true;
        return link;
#endif
    }

    bool ShareText(const std::string &_text)
    {
#if defined(__ANDROID__)
        if (StartShareIntent(_text))
            return true;
        Debug::Warning("Share sheet unavailable, copying instead.");
#endif
        if (SDL_SetClipboardText(_text.c_str()))
            return true;
        Debug::Warning("Could not share or copy text: %s", SDL_GetError());
        return false;
    }
}
