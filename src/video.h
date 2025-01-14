extern int display_mode;

/*Never display borders*/
#define DISPLAY_MODE_NO_BORDERS     0
/*Display borders drawn by VIDC. VIDC often draws no borders on VGA/multisync modes*/
#define DISPLAY_MODE_NATIVE_BORDERS 1
/*Display area drawn by TV-res monitor*/
#define DISPLAY_MODE_TV             2

extern int video_scale;
extern int video_fullscreen_scale;

/**
 * structure used to map host co-ordinates to
 * screen co-ordinates on emulated machine.
 * 
*/
typedef struct {
	// these are all in host pixels, before display scaling
	uint32_t left_border;
	uint32_t top_border;  
	uint32_t screen_width; 
	uint32_t screen_height;

	// these are in RISC OS graphics units
	uint32_t os_unit_width;
	uint32_t os_unit_height;
} ScreenGeometry;

extern ScreenGeometry screen_geom;

void update_screen_geometry(uint32_t lb, uint32_t tb, uint32_t sw, uint32_t sh);

typedef struct
{
    struct { int w, h; } window;
    struct { int x, y, w, h; } viewport;
} video_window_info_t;

video_window_info_t video_window_info();

/* Convert window co-ordinates to RISC OS co-ordinates
 *
 * - w: window info from video_window_info()
 * - wx, wy: window coordinates, 
 * - os_x, os_y: RISC OS screen co-ordinates
 * 
 * Returns 0 if the pointer mapped to a point within the VIDC borders, 
 * or 1 if it was clamped to an edge (either X or Y).
 */
int window_coords_to_os_coords(video_window_info_t video, uint32_t wx, uint32_t wy, short *os_x, short *os_y);


enum
{
	FULLSCR_SCALE_FULL = 0,
	FULLSCR_SCALE_43,
	FULLSCR_SCALE_SQ,
	FULLSCR_SCALE_INT
};

extern int video_linear_filtering;

enum
{
	BLACK_LEVEL_ACORN = 0, /*Crushed black level*/
	BLACK_LEVEL_NORMAL
};

extern int video_black_level;