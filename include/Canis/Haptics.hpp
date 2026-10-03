#pragma once

namespace Canis::Haptics
{
    // Vibrates the device: the phone's motor on Android, or the first
    // rumble-capable haptic device elsewhere. Does nothing without one.
    // _strength runs from 0 to 1.
    void Vibrate(float _strength, unsigned int _milliseconds);
    void Shutdown();
}
