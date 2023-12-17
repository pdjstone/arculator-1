void input_init();
void input_close();

void mouse_poll_host();
void mouse_get_rel(int *x, int *y);
void mouse_get_abs(int *x, int *y, int *b);
int mouse_get_buttons();
int mouse_capture_enable();
void mouse_capture_disable();
//void position_mouse(uint16_t os_x, uint16_t os_y);

void keyboard_poll_host();
extern int key[512];

#include "keyboard_sdl.h"
