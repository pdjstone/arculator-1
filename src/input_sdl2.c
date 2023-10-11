/*Arculator 2.2 by Sarah Walker
  SDL2 input handling*/
#include <SDL2/SDL.h>
#include <string.h>
#include "arc.h"
#include "plat_input.h"
#include "video_sdl2.h"

static int mouse_buttons;
static int mouse_x = 0, mouse_y = 0;

static int mouse_capture = 0;

int mouse[3];

static void mouse_init()
{
    //SDL_SetHintWithPriority(SDL_HINT_MOUSE_RELATIVE_MODE_WARP, "1", SDL_HINT_OVERRIDE);
    //SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_ShowCursor(SDL_DISABLE);
}

static void mouse_close()
{
}

void mouse_capture_enable()
{
	rpclog("Mouse captured\n");
	SDL_SetRelativeMouseMode(SDL_TRUE);
	SDL_SetWindowGrab(sdl_main_window, SDL_TRUE);
	

	// The SDL docs state that calling SDL_SetRelativeMouseMode 
	// "will flush any pending mouse motion" however this doesn't
	// always seem to be the case on Linux, as the first call
	// to mouse_poll_host after entering relative mode will often
	// give a spurious delta. So this call ensures the flush happens
	SDL_GetRelativeMouseState(NULL, NULL);
	mouse_capture = 1;
	mouse_mode = MOUSE_MODE_RELATIVE;
}

void mouse_capture_disable()
{
	rpclog("Mouse released\n");
	mouse_capture = 0;
	SDL_SetWindowGrab(sdl_main_window, SDL_FALSE);
	SDL_SetRelativeMouseMode(SDL_FALSE);
}

void mouse_poll_host()
{
	uint32_t mb = 0;
	if (mouse_capture && mouse_mode == MOUSE_MODE_RELATIVE)
	{
		SDL_Rect rect;
		mb = SDL_GetRelativeMouseState(&mouse[0], &mouse[1]);

		mouse_x += mouse[0];
		mouse_y += mouse[1];

		SDL_GetWindowSize(sdl_main_window, &rect.w, &rect.h);
		SDL_WarpMouseInWindow(sdl_main_window, rect.w/2, rect.h/2);
	} else if (mouse_mode == MOUSE_MODE_ABSOLUTE) 
	{
		mb = SDL_GetMouseState(&mouse_x, &mouse_y);
	}
	else
	{
		mouse_x = mouse_y = mouse_buttons = 0;
	}
	mouse_buttons = 0;
	if (mb & SDL_BUTTON(SDL_BUTTON_LEFT))
	{
		mouse_buttons |= 1;
	}
	if (mb & SDL_BUTTON(SDL_BUTTON_RIGHT))
	{
		mouse_buttons |= 2;
	}
	if (mb & SDL_BUTTON(SDL_BUTTON_MIDDLE))
	{
		mouse_buttons |= 4;
	}
	// printf("mouse %d, %d\n", mouse_x, mouse_y);
}

void mouse_get_rel(int *x, int *y)
{
	*x = mouse_x;
	*y = mouse_y;
	mouse_x = mouse_y = 0;
}

void mouse_get_abs(int *x, int *y, int *b)
{
	*x = mouse_x;
	*y = mouse_y;
	*b = mouse_buttons;
}

int mouse_get_buttons()
{
	return mouse_buttons;
}


int key[512];

static void keyboard_init()
{
}

static void keyboard_close()
{
}

void keyboard_poll_host()
{
	int c;
	const uint8_t *state = SDL_GetKeyboardState(NULL);

	for (c = 0; c < 512; c++)
		key[c] = state[c];
}


void input_init()
{
	mouse_init();
	keyboard_init();
}
void input_close()
{
	keyboard_close();
	mouse_close();
}


/*
This kind of works (tested on Linux) but some games that continually re-position
the mouse cursor make it difficult or impossible to move the host cursor out of the 
Arculator window. 
*/
/*
void position_mouse(uint16_t os_x, uint16_t os_y) 
{
	uint32_t wx, wy;
	os_coords_to_window_coords(os_x, os_y, &wx, &wy);
	rpclog("position_mouse OS(%d,%d) -> Window(%d,%d)\n", os_x, os_y, wx, wy);
	SDL_WarpMouseInWindow(sdl_main_window, wx, wy);

}*/