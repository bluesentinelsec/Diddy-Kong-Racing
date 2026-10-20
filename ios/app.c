// iOS-only host code. Everything else the app needs is the same platform layer
// the desktop builds use (linux/*.c); this file holds what differs on a phone.
//
// It is also the one source Xcode compiles directly into the app target: an
// Xcode target made only of object libraries gets no link step at all, so the
// bundle would ship without an executable.

#include <SDL3/SDL.h>

// Called from gfx_window_init before SDL_Init, when hints still take effect.
void pc_ios_configure(void) {
    // Landscape only, like the Android app (Info.plist lists the same two
    // orientations; this keeps SDL's window agreeing with it).
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeLeft LandscapeRight");
    // The home indicator stays out of the way until swiped for.
    SDL_SetHint(SDL_HINT_IOS_HIDE_HOME_INDICATOR, "2");
}
