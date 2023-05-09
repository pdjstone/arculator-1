extern int skip_video_render;

#define SKIP_VIDEO 1

#define SKIP_VIDEO_DO_ONE_UPDATE 1 << 1
#define SKIP_VIDEO_DO_ONE_PRESENT 1 << 2

#define SKIP_VIDEO_DISPLAY_ONE_FRAME SKIP_VIDEO | SKIP_VIDEO_DO_ONE_UPDATE | SKIP_VIDEO_DO_ONE_PRESENT

static inline void TURBO_RENDER_ONE_FRAME()
{
    
}
static inline int CHECK_SKIP_VIDEO_UPDATE() 
{
    int skip = 0;
	if (skip_video_render) 
    { 
		if (skip_video_render & SKIP_VIDEO_DO_ONE_UPDATE) 
        { 
			skip_video_render = skip_video_render & ~SKIP_VIDEO_DO_ONE_UPDATE; 
		} 
        else 
        { 
			skip = 1; 
		} 
	}
    return skip;
}

static inline int CHECK_SKIP_VIDEO_PRESENT() 
{
    int skip = 0;
	if (skip_video_render) 
    { 
		if (skip_video_render & SKIP_VIDEO_DO_ONE_PRESENT) 
        { 
			skip_video_render = skip_video_render & ~SKIP_VIDEO_DO_ONE_PRESENT; 
		} 
        else 
        { 
			skip = 1; 
		} 
	}
    return skip;
}
