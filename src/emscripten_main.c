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
#include "soundopenal.h"
#include "plat_input.h"
#include "plat_video.h"
#include "vidc.h"
#include "video.h"
#include "video_sdl2.h"

#ifndef __EMSCRIPTEN__
#define EMSCRIPTEN_KEEPALIVE 
#endif

static int winsizex = 0, winsizey = 0;
static int win_doresize = 0;
static int win_dofullscreen = 0;
static int win_dosetresize = 0;
static int win_renderer_reset = 0;


#define TARGET_FPS 60
static int target_delay = 1000/TARGET_FPS;

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

        SDL_Event e;

        while (SDL_PollEvent(&e) != 0)
        {
                if (e.type == SDL_QUIT)
                {
                        arc_stop_main_thread();
                }
                if (e.type == SDL_MOUSEBUTTONUP)
                {
                        if (e.button.button == SDL_BUTTON_LEFT && !mousecapture)
                        {
                                rpclog("Mouse click -- enabling mouse capture\n");
                                sdl_enable_mouse_capture();
                        }
                        else if (e.button.button == SDL_BUTTON_RIGHT && !mousecapture)
                        {
                                //arc_popup_menu();
                        }
                }
                if (e.type == SDL_WINDOWEVENT)
                {
                        switch (e.window.event)
                        {
                                case SDL_WINDOWEVENT_FOCUS_LOST:
                                if (mousecapture)
                                {
                                        rpclog("Focus lost -- disabling mouse capture\n");
                                        sdl_disable_mouse_capture();
                                }
                                break;

                                default:
                                break;
                        }
                }
                if ((key[KEY_LCONTROL] || key[KEY_RCONTROL])
                        && key[KEY_END]
                        && !fullscreen && mousecapture)
                {
                        rpclog("CTRL-END pressed -- disabling mouse capture\n");
                        sdl_disable_mouse_capture();
                }
        }

        /*Resize window to match screen mode*/
        if (!fullscreen && win_doresize)
        {
                SDL_Rect rect;

                win_doresize = 0;

                SDL_GetWindowSize(sdl_main_window, &rect.w, &rect.h);
                if (rect.w != winsizex || rect.h != winsizey)
                {
                        rpclog("Resizing window to %d, %d\n", winsizex, winsizey);
                        SDL_GetWindowPosition(sdl_main_window, &rect.x, &rect.y);
                        SDL_SetWindowSize(sdl_main_window, winsizex, winsizey);
                        SDL_SetWindowPosition(sdl_main_window, rect.x, rect.y);
                }
        }

        /*Toggle fullscreen with RWIN-Enter (Alt-Enter, Cmd-Enter),
                or enter by selecting Fullscreen from the menu.*/
        if (win_dofullscreen ||
                (key[KEY_RWIN] && key[KEY_ENTER] && !fullscreen)
        )
        {
                win_dofullscreen = 0;

                SDL_RaiseWindow(sdl_main_window);
                SDL_SetWindowFullscreen(sdl_main_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
                sdl_enable_mouse_capture();
                fullscreen = 1;
        } else if (fullscreen && (
                /*Exit fullscreen with Ctrl-End*/
                ((key[KEY_LCONTROL] || key[KEY_RCONTROL]) && key[KEY_END])
                /*Toggle with RWIN-Enter*/
                || (key[KEY_RWIN] && key[KEY_ENTER])
        ))
        {
                SDL_SetWindowFullscreen(sdl_main_window, 0);
                sdl_disable_mouse_capture();

                fullscreen=0;
                if (fullborders) updatewindowsize(800,600);
                else             updatewindowsize(672,544);
        }

        if (win_renderer_reset)
        {
                win_renderer_reset = 0;

                if (!video_renderer_reinit(NULL))
                        fatal("Video renderer init failed");
        }

    // Run for 10 ms of processor time
    SDL_LockMutex(main_thread_mutex);
    
    if (!pause_main_thread)
        arc_run(TARGET_FPS);
    SDL_UnlockMutex(main_thread_mutex);

    // Sleep to make it up to 10 ms of real time
    static Uint32 last_timer_ticks = 0;
    static int timer_offset = 0;
    Uint32 current_timer_ticks = SDL_GetTicks();
    Uint32 ticks_since_last = current_timer_ticks - last_timer_ticks;
    last_timer_ticks = current_timer_ticks;
    timer_offset += target_delay - (int)ticks_since_last;
    if (timer_offset > 100 || timer_offset < -100)
    {
            timer_offset = 0;
    }
    else if (timer_offset > 0)
    {
            #ifndef __EMSCRIPTEN__
            SDL_Delay(timer_offset);
            #endif
    }

    if (updatemips)
    {
            char s[80];
            //rpclog("timer_offset now %d; %d ticks since last; delaying %d\n", timer_offset, ticks_since_last, target_delay - ticks_since_last);
            int pct = (int)((inssec / (TARGET_FPS * 1.0)) * 100);
            sprintf(s, "Arculator %s - %i%%", VERSION_STRING, pct);
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
        
        while(!quited) {        
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

void EMSCRIPTEN_KEEPALIVE arc_disc_change(int drive, char *fn)
{
        rpclog("arc_disc_change: drive=%i fn=%s\n", drive, fn);

        SDL_LockMutex(main_thread_mutex);

        disc_close(drive);
        strcpy(discname[drive], fn);
        disc_load(drive, discname[drive]);
        ioc_discchange(drive);

        SDL_UnlockMutex(main_thread_mutex);
}

void EMSCRIPTEN_KEEPALIVE arc_disc_eject(int drive)
{
        rpclog("arc_disc_eject: drive=%i\n", drive);

        SDL_LockMutex(main_thread_mutex);

        ioc_discchange(drive);
        disc_close(drive);
        discname[drive][0] = 0;

        SDL_UnlockMutex(main_thread_mutex);
}

void arc_stop_main_thread()
{
        quited = 1;
        //SDL_WaitThread(main_thread, NULL);
        SDL_DestroyMutex(main_thread_mutex);
        main_thread_mutex = NULL;
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
        al_init_main(0, NULL);
        main_thread_mutex = SDL_CreateMutex();
        arc_main_thread();
}