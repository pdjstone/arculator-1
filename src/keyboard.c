/*Arculator 2.2 by Sarah Walker
  Keyboard/mouse emulation*/
#include <stdio.h>
#include "arc.h"
#include "config.h"
#include "ioc.h"
#include "keyboard.h"
#include "plat_input.h"
#include "keytable.h"
#include "timer.h"
#include "video.h"
#include "vidc.h"

static emu_timer_t keyboard_timer;
static emu_timer_t keyboard_rx_timer;
static emu_timer_t keyboard_tx_timer;

int mouse_mode = MOUSE_MODE_ABSOLUTE;

// is a locked (i.e. relative movements) mouse required? 
int mouse_lock_needed = 0;
int mouse_pointer_linked = 1;

int mousecapture=0;
int ml,mr,mt,mb;

int keydat[512];

int moldx,moldy;
int ledcaps,lednum,ledscr;
int mousedown[3]={0,0,0};
int mouseena=0,keyena=0;
int keystat=0xFF;
int keyrow,keycol;

static uint8_t key_data[2];

enum
{
	KEYBOARD_STANDARD = 0,
	KEYBOARD_A500
};

static int keyboard_type;

enum
{
	/*HRST/RAK1/RAK2 are same on A500 and production keyboards*/
	KEYBOARD_HRST = 0xff,
	KEYBOARD_RAK1 = 0xfe,
	KEYBOARD_RAK2 = 0xfd,

	KEYBOARD_BACK = 0x3f,
	KEYBOARD_BACK_A500 = 0x30
};

enum
{
	KEYBOARD_RESET = 0,
	KEYBOARD_IDLE = 1,
	KEYBOARD_DAT1 = 2,
	KEYBOARD_DAT2 = 3,
	KEYBOARD_DAT_SINGLE = 4
};

void keyboard_send_single(uint8_t val)
{
	LOG_KB_MOUSE("keyboard_send_single %02X\n", val);
	key_data[0] = val;

	keystat = KEYBOARD_DAT_SINGLE;

	timer_set_delay_u64(&keyboard_rx_timer, 1000 * TIMER_USEC);
}

void keyboard_send_double(uint8_t val1, uint8_t val2)
{
	LOG_KB_MOUSE("keyboard_send_double : %02X %02X\n", val1, val2);
	key_data[0] = val1;
	key_data[1] = val2;

	keystat = KEYBOARD_DAT1;

	timer_set_delay_u64(&keyboard_rx_timer, 1000 * TIMER_USEC);
}

void key_do_rx_callback(void *p)
{
	LOG_KB_MOUSE("RX interrupt\n");
	ioc_irqb(IOC_IRQB_KEYBOARD_RX);
}

void key_do_tx_callback(void *p)
{
	LOG_KB_MOUSE("TX interrupt\n");
	ioc_irqb(IOC_IRQB_KEYBOARD_TX);
}

static void keyboard_keydown(int row, int col)
{
	if (keyboard_type == KEYBOARD_STANDARD)
		keyboard_send_double(0xc0 | row, 0xc0 | col);
	else if (keyboard_type == KEYBOARD_A500)
		keyboard_send_double(0x20 | row, 0x20 | col);
}

static void keyboard_keyup(int row, int col)
{
	if (keyboard_type == KEYBOARD_STANDARD)
		keyboard_send_double(0xd0 | row, 0xd0 | col);
	else
		keyboard_send_double(0x30 | row, 0x30 | col);
}

uint8_t keyboard_read()
{
	switch (keystat)
	{
		case KEYBOARD_RESET:
		case KEYBOARD_IDLE:
		return key_data[0];

		case KEYBOARD_DAT_SINGLE:
		keystat = KEYBOARD_IDLE;
		case KEYBOARD_DAT1:
		return key_data[0];
		case KEYBOARD_DAT2:
		return key_data[1];
	}

	return 0;
}

static void keyboard_standard_write(uint8_t val)
{
	int c;

	switch (keystat)
	{
		case KEYBOARD_RESET: /*Reset sequence*/
		LOG_KB_MOUSE("Reset sequence - write %02X\n", val);
		switch (val)
		{
			case KEYBOARD_HRST: /*HRST*/
			keyboard_send_single(KEYBOARD_HRST); /*HRST*/
			for (c = 0; c < 512; c++)
				keydat[c] = 0;
			keyena = mouseena = 0;
			keystat = KEYBOARD_RESET;
			break;
			case KEYBOARD_RAK1: /*RAK1*/
			keyboard_send_single(KEYBOARD_RAK1); /*RAK1*/
			for (c = 0; c < 512; c++)
				keydat[c] = 0;
			keyena = mouseena = 0;
			keystat = KEYBOARD_RESET;
			break;
			case KEYBOARD_RAK2: /*RAK2*/
			keyboard_send_single(KEYBOARD_RAK2); /*RAK2*/
			keystat = KEYBOARD_IDLE;
			break;
		}
		break;

		case KEYBOARD_IDLE: /*Normal*/
//                LOG_KB_MOUSE("Normal - write %02X\n",v);
		switch (val)
		{
			case 0x00: case 0x01: case 0x02: case 0x03: /*Keyboard LEDs*/
			case 0x04: case 0x05: case 0x06: case 0x07:
			ledcaps = val & 1;
			lednum  = val & 2;
			ledscr  = val & 4;
			break;
			case 0x20: /*Keyboard ID*/
			keyboard_send_single(0x81);
			break;
			case 0x30: case 0x31: case 0x32: case 0x33: /*Enable mouse, disable keyboard*/
			if (val & 2) mouseena = 1;
			if (val & 1) keyena = 1;
			break;
			case KEYBOARD_HRST:  /*HRST*/
			keyboard_send_single(KEYBOARD_HRST);
			keystat = KEYBOARD_RESET;
			break;
		}
		break;

		case KEYBOARD_DAT1:
		switch (val)
		{
			case KEYBOARD_BACK:
			keystat = KEYBOARD_DAT2;
			timer_set_delay_u64(&keyboard_rx_timer, 1000 * TIMER_USEC);
			break;

			case KEYBOARD_HRST:
			keyboard_send_single(KEYBOARD_HRST);
			keystat = KEYBOARD_RESET;
			break;
		}
		break;

		case KEYBOARD_DAT2:
		case KEYBOARD_DAT_SINGLE:
		switch (val)
		{
			case 0x30: case 0x31: case 0x32: case 0x33: /*Enable mouse, disable keyboard*/
			if (val & 2) mouseena = 1;
			if (val & 1) keyena = 1;
			keystat = KEYBOARD_IDLE;
			break;

			case KEYBOARD_HRST:
			keyboard_send_single(KEYBOARD_HRST);
			keystat = KEYBOARD_RESET;
			break;
		}
		break;
	}
}

static void keyboard_a500_write(uint8_t val)
{
	int c;

	switch (keystat)
	{
		case KEYBOARD_RESET: /*Reset sequence*/
		LOG_KB_MOUSE("Reset sequence - write %02X\n", val);
		switch (val)
		{
			case KEYBOARD_HRST: /*HRST*/
			keyboard_send_single(KEYBOARD_HRST); /*HRST*/
			for (c = 0; c < 512; c++)
				keydat[c] = 0;
			keyena = mouseena = 0;
			keystat = KEYBOARD_RESET;
			break;
			case KEYBOARD_RAK1: /*RAK1*/
			keyboard_send_single(KEYBOARD_RAK1); /*RAK1*/
			for (c = 0; c < 512; c++)
				keydat[c] = 0;
			keyena = mouseena = 0;
			keystat = KEYBOARD_RESET;
			break;
			case KEYBOARD_RAK2: /*RAK2*/
			keyboard_send_single(KEYBOARD_RAK2); /*RAK2*/
			keystat = KEYBOARD_IDLE;
			break;
		}
		break;

		case KEYBOARD_IDLE: /*Normal*/
//                LOG_KB_MOUSE("Normal - write %02X\n",v);
		switch (val)
		{
			case 0x00: case 0x01: case 0x02: case 0x03: /*Keyboard LEDs*/
			case 0x04: case 0x05: case 0x06: case 0x07:
			ledcaps = val & 1;
			lednum  = val & 2;
			ledscr  = val & 4;
			break;
			case 0x20: /*Keyboard ID*/
			keyboard_send_double(0x10, 0x10);
			break;
			case 0x30: case 0x31: case 0x32: case 0x33: /*Enable mouse, disable keyboard*/
			if (val & 2) mouseena = 1;
			if (val & 1) keyena = 1;
			break;
			case KEYBOARD_HRST:  /*HRST*/
			keyboard_send_single(KEYBOARD_HRST);
			keystat = KEYBOARD_RESET;
			break;
		}
		break;

		case KEYBOARD_DAT1:
		switch (val)
		{
			case KEYBOARD_BACK:
			case KEYBOARD_BACK_A500:
			keystat = KEYBOARD_DAT2;
			timer_set_delay_u64(&keyboard_rx_timer, 1000 * TIMER_USEC);
			break;

			case KEYBOARD_HRST:
			keyboard_send_single(KEYBOARD_HRST);
			keystat = KEYBOARD_RESET;
			break;
		}
		break;

		case KEYBOARD_DAT2:
		case KEYBOARD_DAT_SINGLE:
		switch (val)
		{
			case 0x30: case 0x31: case 0x32: case 0x33: /*Enable mouse, disable keyboard*/
			if (val & 2) mouseena = 1;
			if (val & 1) keyena = 1;
			keystat = KEYBOARD_IDLE;
			break;

			case KEYBOARD_HRST:
			keyboard_send_single(KEYBOARD_HRST);
			keystat = KEYBOARD_RESET;
			break;
		}
		break;
	}
}

void keyboard_write(uint8_t val)
{
	timer_set_delay_u64(&keyboard_tx_timer, 1000 * TIMER_USEC);

	LOG_KB_MOUSE("Keyboard write %02X %i %08X\n", val, keystat, PC);

	if (keyboard_type == KEYBOARD_STANDARD)
		keyboard_standard_write(val);
	else if (keyboard_type == KEYBOARD_A500)
		keyboard_a500_write(val);
}

void keyboard_init()
{
	int c, d;

	keyboard_type = machine_is_a500() ? KEYBOARD_A500 : KEYBOARD_STANDARD;

	keyboard_send_single(KEYBOARD_HRST);
	keystat = KEYBOARD_RESET;
	for (c = 0; c < 512; c++)
	{
		keytable[c][0] = keytable[c][1] = -1;
		keydat[c] = 0;
	}
	c = d = 0;
	while (!d)
	{
		if (keyboard_type == KEYBOARD_STANDARD)
		{
			keytable[keys[c][0] - 1][0] = keys[c][1];
			keytable[keys[c][0] - 1][1] = keys[c][2];
			c++;
			if (keys[c][0] == -1)
				d = 1;
		}
		else if (keyboard_type == KEYBOARD_A500)
		{
			keytable[keys_a500[c][0] - 1][0] = keys_a500[c][1];
			keytable[keys_a500[c][0] - 1][1] = keys_a500[c][2];
			c++;
			if (keys_a500[c][0] == -1)
				d = 1;
		}
	}

	timer_add(&keyboard_timer, keyboard_poll, NULL, 1);
	timer_add(&keyboard_rx_timer, key_do_rx_callback, NULL, 0);
	timer_add(&keyboard_tx_timer, key_do_tx_callback, NULL, 0);

	mouseena = 0;
	keyena = 0;
}

FILE *klog;
void keyboard_poll(void *p)
{
	int mx,my;
	int mouse_buttons;
	int c;
	uint8_t dx, dy;

	timer_advance_u64(&keyboard_timer, TIMER_USEC * 10000);

	/*if (romset > 3)
	{
		ioc_irqb(IOC_IRQB_PODULE_IRQ);
		return;
	}*/
//        LOG_KB_MOUSE("Updatekeys %i %i\n",keystat,keyena);
//        int mouseb=mouse_b;
//        mouse_b|=(key[KEY_MENU])?4:0;

	if (keystat || keyena) LOG_KB_MOUSE("keyboard_poll %i %i\n", keystat, keyena);
	if (keystat != KEYBOARD_IDLE)
		return;
	if (!keyena)
		return;
//        c = poll_keyboard();
	for (c = 1; c < 512; c++)
	{
		if (key[c] != keydat[c] && c != KEY_MENU && keytable[c-1][0] != -1)
		{
			if (key[c])
				keyboard_keydown(keytable[c-1][0], keytable[c-1][1]);
			else
				keyboard_keyup(keytable[c-1][0], keytable[c-1][1]);
			keydat[c] = key[c];
			return;
		}
	}

	mouse_buttons = /*mousecapture ? */mouse_get_buttons()/* : 0*/;

	if ((mouse_buttons & 1) != mousedown[0]) /*Left button*/
	{
		LOG_KB_MOUSE("mouse left click\n");
		mousedown[0] = mouse_buttons & 1;
		if (keyboard_type == KEYBOARD_STANDARD)
		{
			if (mousedown[0])
				keyboard_keydown(7, 0);
			else
				keyboard_keyup(7, 0);
		}
		else if (keyboard_type == KEYBOARD_A500)
		{
			if (mousedown[0])
				keyboard_keydown(0xf, 0);
			else
				keyboard_keyup(0xf, 0);
		}
		return;
	}
	if ((mouse_buttons & 2) != mousedown[1]) /*Right button*/
	{
		LOG_KB_MOUSE("mouse right click\n");
		mousedown[1] = mouse_buttons & 2;
		if (keyboard_type == KEYBOARD_STANDARD)
		{
			if (mousedown[1])
				keyboard_keydown(7, 2);
			else
				keyboard_keyup(7, 2);
		}
		else if (keyboard_type == KEYBOARD_A500)
		{
			if (mousedown[1])
				keyboard_keydown(0xd, 0);
			else
				keyboard_keyup(0xd, 0);
		}
		return;
	}
	/* There are three ways to perform a menu-click in Arculator:
	   - Click the middle mouse button, on a three-button mouse
	   - Press the Menu key (KEY_MENU), on a modern Windows keyboard
	   - Press the left Command key (KEY_LWIN), on a Mac*/
	if (((mouse_buttons & 4) | ((key[KEY_MENU] || key[KEY_LWIN]) ? 4 : 0)) != mousedown[2]) /*Middle button*/
	{
		LOG_KB_MOUSE("mouse middle click / menu key pressed\n");
		mousedown[2] = (mouse_buttons & 4) | ((key[KEY_MENU] || key[KEY_LWIN]) ? 4 : 0);
		if (keyboard_type == KEYBOARD_STANDARD)
		{
			if (mousedown[2])
				keyboard_keydown(7, 1);
			else
				keyboard_keyup(7, 1);
		}
		else if (keyboard_type == KEYBOARD_A500)
		{
			if (mousedown[2])
				keyboard_keydown(0xe, 0);
			else
				keyboard_keyup(0xe, 0);
		}
		return;
	}

	if (mouse_lock_needed && vidc_cursor_visible() && mouse_pointer_linked)
		set_mouse_lock_needed(0);

	if (mouseena && mouse_mode == MOUSE_MODE_RELATIVE/* && mousecapture*/)// && (!mousehack || fullscreen))
	{
		mouse_get_rel(&mx,&my);
//                if (mousecapture && !fullscreen)
//                        position_mouse(320,256);

		if (!mx && !my)
			return;

		if (keyboard_type == KEYBOARD_STANDARD)
		{
			if (mx < -63)
				dx = -63;
			else if (mx > 63)
				dx = 63;
			else
				dx = mx;

			if (my < -63)
				dy = -63;
			else if (my > 63)
				dy = 63;
			else
				dy = my;

			keyboard_send_double(dx & 0x7f, (-dy) & 0x7f);
		}
		else if (keyboard_type == KEYBOARD_A500)
		{
			if (mx < -31)
				dx = -31;
			else if (mx > 31)
				dx = 31;
			else
				dx = mx;

			if (my < -31)
				dy = -31;
			else if (my > 31)
				dy = 31;
			else
				dy = my;

			keyboard_send_double((dx & 0x3f) | 0x40, ((-dy) & 0x3f) | 0x40);
		}

		LOG_KB_MOUSE("Update mouse %i %i %i\n", mousex, mousey, keystat);
	}
}

void doosmouse()
{
	short xpos, ypos;
	int mx, my, b;

	if (mouse_mode == MOUSE_MODE_RELATIVE || fullscreen || !mouseena) 
		return;

	LOG_KB_MOUSE("doosmouse\n");
	mouse_get_abs(&mx, &my, &b);

    window_coords_to_os_coords(video_window_info(), mx, my, &xpos, &ypos);

	if (ypos<mt) ypos=mt;
	if (ypos>mb) ypos=mb;
	writememl(0x5B8,ypos);

	if (xpos>mr) xpos=mr;
	if (xpos<ml) xpos=ml;
	writememl(0x5B4,xpos);
	
	//rpclog("doosmouse x=%d, y=%d, my=%d, offsety=%d, mouseena=%d\n", xpos, ypos, my, offsety, mouseena);
}

void setmousepos(uint32_t a)
{
	//uint16_t x,y;
	//x=readmemb(a+1)|(readmemb(a+2)<<8);
	//y=readmemb(a+3)|(readmemb(a+4)<<8);
	//LOG_KB_MOUSE("setmousepos x=%d y=%d\n", x, y);
	//position_mouse(x,y);
	set_mouse_lock_needed(!vidc_cursor_visible() || !mouse_pointer_linked);
}

void getunbufmouse(uint32_t a)
{
	short xpos,ypos;
	int mx, my, b;
	mouse_get_abs(&mx,&my, &b);
	LOG_KB_MOUSE("getunbufmouse\n");
	
	rpclog("getunbufmouse mouseena=%d\n", mouseena);
	ypos=(my+offsety)<<1;
	if (ypos<mt) ypos=mt;
	if (ypos>mb) ypos=mb;
	writememb(a+1,ypos&0xFF);
	writememb(a+2,(ypos>>8)&0xFF);
	xpos=(mx-offsetx)<<1;
	if (xpos>mr) xpos=mr;
	if (xpos<ml) xpos=ml;
	writememb(a+3,xpos&0xFF);
	writememb(a+4,(xpos>>8)&0xFF);
}

void getosmouse()
{
	long xpos, ypos, buttons;
	int mx, my, mouse_b;
	mouse_get_abs(&mx,&my,&mouse_b);
	LOG_KB_MOUSE("getosmouse\n");
	
	ypos=(my+offsety)<<1;
	if (ypos<mt) ypos=mt;
	if (ypos>mb) ypos=mb;
	armregs[1]=ypos;
	xpos=(mx-offsetx)<<1;
	if (xpos>mr) xpos=mr;
	if (xpos<ml) xpos=ml;
	armregs[0]=xpos;
	buttons=0;
	if (mouse_b&1) buttons|=4;
	if (mouse_b&2) buttons|=1;
	if (mouse_b&4) buttons|=2;
	
	armregs[2]=buttons;
	armregs[3]=0;
	 
	rpclog("getosmouse %d,%d,%d mousena=%d\n", xpos, ypos, buttons, mouseena);
	LOG_KB_MOUSE("%08X %08X %08X\n",armregs[0],armregs[1],armregs[2]);
}

void setmousebounds(uint32_t a)
{
	LOG_KB_MOUSE("setmousebounds\n");
	
	ml=(short)(readmemb(a+1)|(readmemb(a+2)<<8));
	mt=(short)(readmemb(a+3)|(readmemb(a+4)<<8));
	mr=(short)(readmemb(a+5)|(readmemb(a+6)<<8));
	mb=(short)(readmemb(a+7)|(readmemb(a+8)<<8));

	rpclog("setmousebounds (%d,%d) (%d,%d)\n", ml, mt, mr, mb);
}

void setmousecursor(uint32_t a)
{
	int pointer_shape = a & 0xf;
	int linked = !(a & 0x80);
	mouse_pointer_linked = linked;
	rpclog("setmousecursor shape=%d linked=%d\n", pointer_shape, linked);
}

void resetmouse()
{
	ml=mt=0;
	mr=0x4FF;
	mb=0x3FF;
}

void set_mouse_lock_needed(uint32_t needed)
{
	if (needed && !mouse_lock_needed) {
		int x,y;
		mouse_mode = MOUSE_MODE_RELATIVE;
		mouse_get_rel(&x, &y); // read relative mouse to zero and existing value
		rpclog("need to grab mouse lock - mousecapture=%d\n", mousecapture);
	} else if (!needed && mouse_lock_needed) {
		rpclog("mouse lock no longer needed\n");
		mouse_mode = MOUSE_MODE_ABSOLUTE;
		mouse_capture_disable();
		mousecapture=0;
		// TODO: when we release the mouse/show the host mouse, SDL sets it to 
		// be in the centre of the window. Should we set it to be somewhere else,
		// e.g. where guest mouse is?
	}
	mouse_lock_needed = needed;
}

