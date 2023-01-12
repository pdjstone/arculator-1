void keyboard_init();
void keyboard_poll(void *p);
uint8_t keyboard_read();
void keyboard_write(uint8_t val);

void key_do_rx_callback(void *p);
void key_do_tx_callback(void *p);

void resetmouse();
void doosmouse();
void setmousepos(uint32_t a);
void getunbufmouse(uint32_t a);
void getosmouse();
void setmouseparams(uint32_t a);
