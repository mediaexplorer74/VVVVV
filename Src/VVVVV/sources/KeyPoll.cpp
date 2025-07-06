#define KEY_DEFINITION
#include <SDL_main.h>
#include <SDL.h>
#include <SDL_mixer.h>
#include <stdio.h>
#include <string.h>

#include <utf8/unchecked.h>

#include "KeyPoll.h"
#include "Game.h"
#include "Graphics.h"
#include "Music.h"

void KeyPoll::setSensitivity(int _value)
{
	switch (_value)
	{
		case 0:
			sensitivity = 28000;
			break;
		case 1:
			sensitivity = 16000;
			break;
		case 2:
			sensitivity = 8000;
			break;
		case 3:
			sensitivity = 4000;
			break;
		case 4:
			sensitivity = 2000;
			break;
	}

}

KeyPoll::KeyPoll()
{
	xVel = 0;
	yVel = 0;
	setSensitivity(2);

	quitProgram = 0;
	keybuffer="";
	leftbutton=0; rightbutton=0; middlebutton=0;
	mx=0; my=0;
	resetWindow = 0;
	toggleFullscreen = false;
	pressedbackspace=false;

	useFullscreenSpaces = false;
	if (strcmp(SDL_GetPlatform(), "Mac OS X") == 0)
	{
		useFullscreenSpaces = true;
		const char *hint = SDL_GetHint(SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES);
		if (hint != NULL)
		{
			useFullscreenSpaces = (strcmp(hint, "1") == 0);
		}
	}

	linealreadyemptykludge = false;

	pauseStart = 0;

	isActive = true;
}

void KeyPoll::enabletextentry()
{
	keybuffer="";
	SDL_StartTextInput();
}

void KeyPoll::disabletextentry()
{
	SDL_StopTextInput();
}

bool KeyPoll::textentry()
{
	return SDL_IsTextInputActive() == SDL_TRUE;
}

void KeyPoll::ClearTouchGesture() {
    touch_swipe_left = touch_swipe_right = touch_swipe_up = touch_swipe_down = touch_tap = false;
}
void KeyPoll::ClearMouseGesture() {
    mouse_swipe_left = mouse_swipe_right = mouse_swipe_up = mouse_swipe_down = mouse_tap = mouse_jump = false;
}

void KeyPoll::Poll()
{
	bool altpressed = false;
	SDL_Event evt;
	while (SDL_PollEvent(&evt))
	{
		switch (evt.type)
		{
		/* Keyboard Input */
		case SDL_KEYDOWN:
		{
			keymap[evt.key.keysym.sym] = true;

			if (evt.key.keysym.sym == SDLK_BACKSPACE)
			{
				pressedbackspace = true;
			}

#ifdef __APPLE__ /* OSX prefers the command keys over the alt keys. -flibit */
			altpressed = keymap[SDLK_LGUI] || keymap[SDLK_RGUI];
#else
			altpressed = keymap[SDLK_LALT] || keymap[SDLK_RALT];
#endif
			bool returnpressed = evt.key.keysym.sym == SDLK_RETURN;
			bool fpressed = evt.key.keysym.sym == SDLK_f;
			bool f11pressed = evt.key.keysym.sym == SDLK_F11;
			if ((altpressed && (returnpressed || fpressed)) || f11pressed)
			{
				toggleFullscreen = true;
			}

			if (textentry())
			{
				if (evt.key.keysym.sym == SDLK_BACKSPACE && !keybuffer.empty())
				{
					std::string::iterator iter = keybuffer.end();
					utf8::unchecked::prior(iter);
					keybuffer = keybuffer.substr(0, iter - keybuffer.begin());
					if (keybuffer.empty())
					{
						linealreadyemptykludge = true;
					}
				}
				else if (	evt.key.keysym.sym == SDLK_v &&
						keymap[SDLK_LCTRL]	)
				{
					keybuffer += SDL_GetClipboardText();
				}
			}
			break;
		}
		case SDL_KEYUP:
			keymap[evt.key.keysym.sym] = false;
			if (evt.key.keysym.sym == SDLK_BACKSPACE)
			{
				pressedbackspace = false;
			}
			break;
		case SDL_TEXTINPUT:
			if (!altpressed)
			{
				keybuffer += evt.text.text;
			}
			break;

		/* Mouse Input */
		case SDL_MOUSEMOTION:
			mx = evt.motion.x;
			my = evt.motion.y;
			if (mouse_drag_active) {
				mouse_end_x = evt.motion.x;
				mouse_end_y = evt.motion.y;
				mouse_end_time = SDL_GetTicks();
			}
			break;
		case SDL_MOUSEBUTTONDOWN:
			switch (evt.button.button)
			{
			case SDL_BUTTON_LEFT:
				mx = evt.button.x;
				my = evt.button.y;
				leftbutton = 1;
				mouse_drag_active = true;
				mouse_start_x = evt.button.x;
				mouse_start_y = evt.button.y;
				mouse_start_time = SDL_GetTicks();
				break;
			case SDL_BUTTON_RIGHT:
				mx = evt.button.x;
				my = evt.button.y;
				rightbutton = 1;
				break;
			case SDL_BUTTON_MIDDLE:
				mx = evt.button.x;
				my = evt.button.y;
				middlebutton = 1;
				break;
			}
			break;
		case SDL_MOUSEBUTTONUP:
			switch (evt.button.button)
			{
			case SDL_BUTTON_LEFT:
				mx = evt.button.x;
				my = evt.button.y;
				leftbutton=0;
				mouse_drag_active = false;
				mouse_end_x = evt.button.x;
				mouse_end_y = evt.button.y;
				mouse_end_time = SDL_GetTicks();
				{
					int dx = mouse_end_x - mouse_start_x;
					int dy = mouse_end_y - mouse_start_y;
					Uint32 dt = mouse_end_time - mouse_start_time;
					const int SWIPE_THRESHOLD = 50;
					const Uint32 TAP_TIME = 200;
					ClearMouseGesture();
					if (abs(dx) < SWIPE_THRESHOLD && abs(dy) < SWIPE_THRESHOLD && dt < TAP_TIME) {
						mouse_tap = true;
						mouse_jump = true;
					} else if (abs(dx) > abs(dy) && abs(dx) > SWIPE_THRESHOLD) {
						if (dx > 0) mouse_swipe_right = true;
						else mouse_swipe_left = true;
					} else if (abs(dy) > SWIPE_THRESHOLD) {
						if (dy > 0) mouse_swipe_down = true;
						else mouse_swipe_up = true;
					}
				}
				break;
			case SDL_BUTTON_RIGHT:
				mx = evt.button.x;
				my = evt.button.y;
				rightbutton=0;
				break;
			case SDL_BUTTON_MIDDLE:
				mx = evt.button.x;
				my = evt.button.y;
				middlebutton=0;
				break;
			}
			break;

		/* Touch Input */
		case SDL_FINGERDOWN:
			touch_active = true;
			touch_start_x = evt.tfinger.x;
			touch_start_y = evt.tfinger.y;
			touch_start_time = SDL_GetTicks();
			break;
		case SDL_FINGERUP:
			touch_active = false;
			touch_end_x = evt.tfinger.x;
			touch_end_y = evt.tfinger.y;
			touch_end_time = SDL_GetTicks();
			{
				float dx = touch_end_x - touch_start_x;
				float dy = touch_end_y - touch_start_y;
				Uint32 dt = touch_end_time - touch_start_time;
				const float SWIPE_THRESHOLD = 0.05f; // Normalized (0..1)
				const Uint32 TAP_TIME = 200;
				ClearTouchGesture();
				if (fabs(dx) < SWIPE_THRESHOLD && fabs(dy) < SWIPE_THRESHOLD && dt < TAP_TIME) {
					touch_tap = true;
				} else if (fabs(dx) > fabs(dy) && fabs(dx) > SWIPE_THRESHOLD) {
					if (dx > 0) touch_swipe_right = true;
					else touch_swipe_left = true;
				} else if (fabs(dy) > SWIPE_THRESHOLD) {
					if (dy > 0) touch_swipe_down = true;
					else touch_swipe_up = true;
				}
			}
			break;
		case SDL_FINGERMOTION:
			// Optional: could update touch_end_x/y for live feedback
			break;

		/* Controller Input */
		case SDL_CONTROLLERBUTTONDOWN:
			buttonmap[(SDL_GameControllerButton) evt.cbutton.button] = true;
			break;
		case SDL_CONTROLLERBUTTONUP:
			buttonmap[(SDL_GameControllerButton) evt.cbutton.button] = false;
			break;
		case SDL_CONTROLLERAXISMOTION:
			switch (evt.caxis.axis)
			{
			case SDL_CONTROLLER_AXIS_LEFTX:
				if (	evt.caxis.value > -sensitivity &&
					evt.caxis.value < sensitivity	)
				{
					xVel = 0;
				}
				else
				{
					xVel = (evt.caxis.value > 0) ? 1 : -1;
				}
				break;
			case SDL_CONTROLLER_AXIS_LEFTY:
				if (	evt.caxis.value > -sensitivity &&
					evt.caxis.value < sensitivity	)
				{
					yVel = 0;
				}
				else
				{
					yVel = (evt.caxis.value > 0) ? 1 : -1;
				}
				break;
			}
			break;
		case SDL_CONTROLLERDEVICEADDED:
		{
			SDL_GameController *toOpen = SDL_GameControllerOpen(evt.cdevice.which);
			printf(
				"Opened SDL_GameController ID #%i, %s\n",
				evt.cdevice.which,
				SDL_GameControllerName(toOpen)
			);
			controllers[SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(toOpen))] = toOpen;
			break;
		}
		case SDL_CONTROLLERDEVICEREMOVED:
		{
			SDL_GameController *toClose = controllers[evt.cdevice.which];
			controllers.erase(evt.cdevice.which);
			printf("Closing %s\n", SDL_GameControllerName(toClose));
			SDL_GameControllerClose(toClose);
			break;
		}

		/* Window Events */
		case SDL_WINDOWEVENT:
			switch (evt.window.event)
			{
			/* Window Resize */
			case SDL_WINDOWEVENT_RESIZED:
				resetWindow = true;
				break;

			/* Window Focus */
			case SDL_WINDOWEVENT_FOCUS_GAINED:
				if (!game.disablepause)
				{
					isActive = true;
				}
				if (!useFullscreenSpaces)
				{
					if (wasFullscreen)
					{
						graphics.screenbuffer->isWindowed = false;
						SDL_SetWindowFullscreen(
							SDL_GetWindowFromID(evt.window.windowID),
							SDL_WINDOW_FULLSCREEN_DESKTOP
						);
					}
				}
				SDL_DisableScreenSaver();
				if (!game.disablepause && Mix_PlayingMusic())
				{
					// Correct songStart for how long we were paused
					music.songStart += SDL_GetPerformanceCounter() - pauseStart;
				}
				break;
			case SDL_WINDOWEVENT_FOCUS_LOST:
				if (!game.disablepause)
				{
					isActive = false;
				}
				if (!useFullscreenSpaces)
				{
					wasFullscreen = !graphics.screenbuffer->isWindowed;
					graphics.screenbuffer->isWindowed = true;
					SDL_SetWindowFullscreen(
						SDL_GetWindowFromID(evt.window.windowID),
						0
					);
				}
				SDL_EnableScreenSaver();
				if (!game.disablepause)
				{
					pauseStart = SDL_GetPerformanceCounter();
				}
				break;

			/* Mouse Focus */
			case SDL_WINDOWEVENT_ENTER:
				SDL_DisableScreenSaver();
				break;
			case SDL_WINDOWEVENT_LEAVE:
				SDL_EnableScreenSaver();
				break;
			}
			break;

		/* Quit Event */
		case SDL_QUIT:
			quitProgram = true;
			break;
		}
	}
}

bool KeyPoll::isDown(SDL_Keycode key)
{
	return keymap[key];
}

bool KeyPoll::isUp(SDL_Keycode key)
{
	return !keymap[key];
}

bool KeyPoll::isDown(std::vector<SDL_GameControllerButton> buttons)
{
	for (size_t i = 0; i < buttons.size(); i += 1)
	{
		if (buttonmap[buttons[i]])
		{
			return true;
		}
	}
	return false;
}

bool KeyPoll::isDown(SDL_GameControllerButton button)
{
	return buttonmap[button];
}

bool KeyPoll::controllerButtonDown()
{
	for (
		SDL_GameControllerButton button = SDL_CONTROLLER_BUTTON_A;
		button < SDL_CONTROLLER_BUTTON_DPAD_UP;
		button = (SDL_GameControllerButton) (button + 1)
	) {
		if (isDown(button))
		{
			return true;
		}
	}
	return false;
}

bool KeyPoll::controllerWantsLeft(bool includeVert)
{
	return (	buttonmap[SDL_CONTROLLER_BUTTON_DPAD_LEFT] ||
			xVel < 0 ||
			(	includeVert &&
				(	buttonmap[SDL_CONTROLLER_BUTTON_DPAD_UP] ||
					yVel < 0	)	)	);
}

bool KeyPoll::controllerWantsRight(bool includeVert)
{
	return (	buttonmap[SDL_CONTROLLER_BUTTON_DPAD_RIGHT] ||
			xVel > 0 ||
			(	includeVert &&
				(	buttonmap[SDL_CONTROLLER_BUTTON_DPAD_DOWN] ||
					yVel > 0	)	)	);
}
