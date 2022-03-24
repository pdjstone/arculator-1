/*Arculator 2.1 by Sarah Walker
  Generic SDL-based main window handling*/
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif
#include "arc.h"
#include "disc.h"
#include "ioc.h"
#include "plat_input.h"
#include "plat_video.h"
#include "vidc.h"
#include "video.h"
#include "video_sdl2.h"

static int winsizex = 0, winsizey = 0;
static int win_doresize = 0;
static int win_dofullscreen = 0;
static int win_dosetresize = 0;
static int win_renderer_reset = 0;

void updatewindowsize(int x, int y)
{
        winsizex = x; winsizey = y;
        win_doresize = 1;
}

static void sdl_enable_mouse_capture()
{
        mouse_capture_enable();
        SDL_SetWindowGrab(sdl_main_window, SDL_TRUE);
        mousecapture = 1;
        updatemips = 1;
}

static void sdl_disable_mouse_capture()
{
        SDL_SetWindowGrab(sdl_main_window, SDL_FALSE);
        mouse_capture_disable();
        mousecapture = 0;
        updatemips = 1;
}

static volatile int quited = 0;
static volatile int pause_main_thread = 0;

static SDL_mutex *main_thread_mutex = NULL;

static time_t last_seconds = 0;
void arcloop()
{
    struct timeval tp;
    
    //rpclog("Aevent loop\n");
    
    if (gettimeofday(&tp, NULL) == -1)
    {
            perror("gettimeofday");
            fatal("gettimeofday failed\n");
    }
    else if (!last_seconds)
    {
            last_seconds = tp.tv_sec;
            rpclog("start time = %d\n", last_seconds);
    }
    else if (last_seconds != tp.tv_sec)
    {
            //rpclog("calling updateins");
            updateins();
            last_seconds = tp.tv_sec;
    }

    if (win_renderer_reset)
    {
            rpclog("a1");
        win_renderer_reset = 0;

        if (!video_renderer_reinit(NULL))
                fatal("Video renderer init failed");
    }

    // Run for 10 ms of processor time
    //SDL_LockMutex(main_thread_mutex);
    
    if (!pause_main_thread)
        arc_run();
    //SDL_UnlockMutex(main_thread_mutex);

    // Sleep to make it up to 10 ms of real time
    static Uint32 last_timer_ticks = 0;
    static int timer_offset = 0;
    Uint32 current_timer_ticks = SDL_GetTicks();
    Uint32 ticks_since_last = current_timer_ticks - last_timer_ticks;
    last_timer_ticks = current_timer_ticks;
    timer_offset += 10 - (int)ticks_since_last;
    // rpclog("timer_offset now %d; %d ticks since last; delaying %d\n", timer_offset, ticks_since_last, 10 - ticks_since_last);
    if (timer_offset > 100 || timer_offset < -100)
    {
            timer_offset = 0;
    }
    else if (timer_offset > 0)
    {
            //SDL_Delay(timer_offset);
    }

    if (updatemips)
    {
            char s[80];

            sprintf(s, "Arculator %s - %i%%", VERSION_STRING, inssec);
            vidc_framecount = 0;
            if (!fullscreen)
            SDL_SetWindowTitle(sdl_main_window, s);
            updatemips=0;
    }

  
}

static int arc_main_thread()
{
        rpclog("Arculator startup\n");
      
        last_seconds = 0;
        arc_init();

        
        if (!video_renderer_init(NULL))
        {
                fatal("Video renderer init failed");
        }
        input_init(); 
        sdl_enable_mouse_capture();
        #ifdef __EMSCRIPTEN__
        emscripten_set_main_loop(arcloop, 0, 1);
        #else
        while(1) {        
                arcloop();
        }
        #endif 
        return 0;
}



void arc_do_reset()
{
        SDL_LockMutex(main_thread_mutex);
        arc_reset();
        SDL_UnlockMutex(main_thread_mutex);
}

void arc_disc_change(int drive, char *fn)
{
        rpclog("arc_disc_change: drive=%i fn=%s\n", drive, fn);

        SDL_LockMutex(main_thread_mutex);

        disc_close(drive);
        strcpy(discname[drive], fn);
        disc_load(drive, discname[drive]);
        ioc_discchange(drive);

        SDL_UnlockMutex(main_thread_mutex);
}

void arc_disc_eject(int drive)
{
        rpclog("arc_disc_eject: drive=%i\n", drive);

        SDL_LockMutex(main_thread_mutex);

        ioc_discchange(drive);
        disc_close(drive);
        discname[drive][0] = 0;

        SDL_UnlockMutex(main_thread_mutex);
}

void arc_renderer_reset()
{
        win_renderer_reset = 1;
}

void arc_set_display_mode(int new_display_mode)
{
        SDL_LockMutex(main_thread_mutex);

        display_mode = new_display_mode;
        clearbitmap();
        setredrawall();

        SDL_UnlockMutex(main_thread_mutex);
}

void arc_set_dblscan(int new_dblscan)
{
        SDL_LockMutex(main_thread_mutex);

        dblscan = new_dblscan;
        clearbitmap();

        SDL_UnlockMutex(main_thread_mutex);
}

void arc_set_resizeable()
{
        win_dosetresize = 1;
}

void arc_enter_fullscreen()
{
        win_dofullscreen = 1;
}

int main() {
    arc_main_thread();
}