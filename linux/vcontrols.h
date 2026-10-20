#ifndef LINUX_VCONTROLS_H
#define LINUX_VCONTROLS_H

// On-screen touch controls for phones and tablets.
//
// A virtual left analog stick (steering) sits at the bottom-left; three action
// buttons in a triangle at the bottom-right drive accelerate, brake and use-item;
// a pause button sits at the bottom-centre. Cruise control: while driving, a
// double tap on the accelerator latches it (the button turns solid with two
// bars, and pulses); a single tap or the brake releases it. It cannot engage in
// a menu and drops when driving stops, so A never stays held into a menu. The layer is drawn as a GL overlay
// above the presented frame (vcontrols_render), fed from SDL touch/mouse events
// (vcontrols_event), and merged into the host controller state the game reads
// (vcontrols_merge). It is on only on mobile: Android (Kindle Fire included),
// iOS, and the browser build on a touch-first device. DKR_VCONTROLS=1 forces it
// on anywhere (the mouse then stands in for one finger, for testing) and
// DKR_VCONTROLS=0 forces it off.

#include <SDL3/SDL.h>

// 1 when the overlay should run on this host.
int vcontrols_active(void);

// Update the control state from one SDL event. winW/winH are the window size in
// pixels (as gfx tracks it). Ignores events it does not care about.
void vcontrols_event(const SDL_Event *e, int winW, int winH);

// Draw the overlay into the default framebuffer. Call after the scene is on the
// window and before SDL_GL_SwapWindow. Lazily builds its own GL program/VAO.
void vcontrols_render(int winW, int winH);

// OR the virtual buttons into *button and, when the stick is being touched,
// replace *stickX/*stickY with the virtual stick (-80..80, y up). A real
// gamepad or keyboard still works when the screen is not being touched.
void vcontrols_merge(unsigned short *button, signed char *stickX, signed char *stickY);

#endif
