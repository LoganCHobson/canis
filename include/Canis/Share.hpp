#pragma once

#include <string>

namespace Canis
{
    // Hands text to the platform's share sheet on Android. Elsewhere it is
    // copied to the clipboard. Returns false if neither worked.
    bool ShareText(const std::string &_text);

    // The link the app was opened with (Android App Links, see
    // Project Settings > Android > Links), returned once and then cleared.
    // Empty when there is none. Desktop builds read CANIS_OPEN_LINK instead,
    // for testing.
    std::string TakeOpenedLink();
}
