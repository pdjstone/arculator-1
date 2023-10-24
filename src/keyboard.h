void keyboard_init();
void keyboard_poll();
uint8_t keyboard_read();
void keyboard_write(uint8_t val);

void key_do_rx_callback();
void key_do_tx_callback();

void resetmouse();
void doosmouse();
void setmousepos(uint32_t a);
void getunbufmouse(uint32_t a);
void getosmouse();
void setmousebounds(uint32_t a);
void set_mouse_lock_needed(uint32_t needed);
