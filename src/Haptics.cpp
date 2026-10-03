#include <Canis/Haptics.hpp>
#include <Canis/Debug.hpp>

#include <SDL3/SDL.h>

#include <algorithm>

namespace Canis::Haptics
{
    namespace
    {
        SDL_Haptic *haptic = nullptr;
        bool initialized = false;

        // Opened on first use so games that never vibrate never touch it.
        SDL_Haptic *Device()
        {
            if (initialized)
                return haptic;
            initialized = true;

            if (!SDL_InitSubSystem(SDL_INIT_HAPTIC))
            {
                Debug::Warning("Haptics unavailable: %s", SDL_GetError());
                return nullptr;
            }

            int count = 0;
            SDL_HapticID *devices = SDL_GetHaptics(&count);
            if (devices != nullptr && count > 0)
                haptic = SDL_OpenHaptic(devices[0]);
            SDL_free(devices);

            if (haptic != nullptr && !SDL_InitHapticRumble(haptic))
            {
                SDL_CloseHaptic(haptic);
                haptic = nullptr;
            }
            return haptic;
        }
    }

    void Vibrate(float _strength, unsigned int _milliseconds)
    {
        if (SDL_Haptic *device = Device())
            SDL_PlayHapticRumble(device, std::clamp(_strength, 0.0f, 1.0f), _milliseconds);
    }

    void Shutdown()
    {
        if (haptic != nullptr)
            SDL_CloseHaptic(haptic);
        haptic = nullptr;
        initialized = false;
    }
}
