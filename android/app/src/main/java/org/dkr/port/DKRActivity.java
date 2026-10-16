package org.dkr.port;

import org.libsdl.app.SDLActivity;

// Diddy Kong Racing on Android. SDLActivity loads libSDL3.so then libmain.so
// and enters the game through SDL_main (linux/main.c). The on-screen touch
// controls and everything else live in the native code.
public class DKRActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "SDL3", "main" };
    }
}
