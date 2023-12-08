/*Arculator 2.2 by Sarah Walker

  SDL2 + OpenGL video handling 
*/

#define GL_GLEXT_PROTOTYPES
#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

#if WIN32
#define BITMAP __win_bitmap
#include <windows.h>
#undef BITMAP
#endif

#include <stdio.h>
#include <string.h>
#include "arc.h"
#include "plat_video.h"
#include "vidc.h"
#include "video.h"
#include "video_sdl2.h"
#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

// other SDL code references this
SDL_Window *sdl_main_window = NULL;

static SDL_GLContext context;
static GLuint screenTexture;

// FIXME: vestigial bits left in to avoid changes to other files
int selected_video_renderer;
int video_renderer_get_id(char *name) { return 0; }
char *video_renderer_get_name(int id) { return "SDL2+OpenGL"; }
int video_renderer_reinit(void *unused) { return 0; }
// FIXME

int skip_video_render = 0;
int take_screenshot = 0;
int record_video = 0;

void EMSCRIPTEN_KEEPALIVE arc_capture_screenshot() {
	take_screenshot = 1;
}

void EMSCRIPTEN_KEEPALIVE arc_capture_video(int record) {
	if (record) {
		rpclog("arc_capture_video: start recording\n");
	} else {
		rpclog("arc_capture_video: stop recording\n");
	}
	record_video = record;
}

#ifdef __EMSCRIPTEN__
#  define GL_ERROR_REPORT rpclog("GL general error @ %s:%d: code %d\n", __FILE__, __LINE__, err)
#else
#  include <GL/glu.h>
#  define GL_ERROR_REPORT rpclog("GL general error @ %s:%d: %s\n", __FILE__, __LINE__, gluErrorString(err))
#endif

/* Horrible macros for error reporting */
#define CHECK_GL_ERROR                                                         \
    do                                                                         \
    {                                                                          \
        GLenum err;                                                            \
        if ((err = glGetError()) != GL_NO_ERROR) { GL_ERROR_REPORT; abort(); } \
    } while (0)

#define CHECK_GL_COMPILE_ERROR(f, s, t)                             \
    do                                                              \
    {                                                               \
        GLint success;                                              \
        (f)((s), (t), &success);                                    \
        CHECK_GL_ERROR;                                             \
        if (success == GL_FALSE)                                    \
        {                                                           \
            GLchar infoLog[512] = "?\0";                            \
            glGetShaderInfoLog((s), 512, NULL, infoLog);            \
            rpclog("#t error for shader %d @ %s:%d: %s\n", (s), __FILE__, __LINE__, infoLog); \
            return SDL_FALSE;                                       \
        }                                                           \
    } while (0)

/* Object id for our shader program */
GLuint shaderProgram;
/* Uniform index for the "zoom" vec4 */
GLuint monitorZoomLoc;
/* Vertex array object listing virtual monitor coordinates */
GLuint monitorVao;

/* This is probably too large and needs breaking up to cope with rebuilding the GL context in case it's lost (?) */
int video_renderer_init(void *unused)
{
    /* SDL and OpenGL globals */
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

#ifdef __EMSCRIPTEN__
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
#else
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#endif

    /* Create the window, OpenGL context */

    sdl_main_window = SDL_CreateWindow(
        "Arculator",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        1024,
        768,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_OPENGL);
        /* HIDPI seems counter-productive, am I missing anything here? */

    if (!sdl_main_window)
	{
        rpclog("SDL_CreateWindowFrom could not be created! Error: %s\n",
            SDL_GetError());
		return SDL_FALSE;
	}

    context = SDL_GL_CreateContext(sdl_main_window);
    if (!context)
    {
        rpclog("SDL_GL_CreateContext failed: %s\n", SDL_GetError());
        return SDL_FALSE;
    }
    CHECK_GL_ERROR;

    /* Compile the vertex and fragment shaders */

#include "video.vert.c"
#include "video.frag.c"

    GLchar const *sources[] = {(const GLchar *)&src_video_vert_glsl, (const GLchar *) &src_video_frag_glsl};
    GLint lengths[] = {src_video_vert_glsl_len, src_video_frag_glsl_len};

    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER); CHECK_GL_ERROR;
    glShaderSource(vertexShader, 1, sources, lengths);
    CHECK_GL_ERROR;
    glCompileShader(vertexShader); 
    CHECK_GL_COMPILE_ERROR(glGetShaderiv, vertexShader, GL_COMPILE_STATUS);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER); CHECK_GL_ERROR;
    glShaderSource(fragmentShader, 1, sources+1, lengths+1);
    CHECK_GL_ERROR;
    glCompileShader(fragmentShader); CHECK_GL_ERROR;
    CHECK_GL_COMPILE_ERROR(glGetShaderiv, fragmentShader, GL_COMPILE_STATUS);

    shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader); CHECK_GL_ERROR;
    glAttachShader(shaderProgram, fragmentShader); CHECK_GL_ERROR;
    glLinkProgram(shaderProgram); CHECK_GL_ERROR;
    CHECK_GL_COMPILE_ERROR(glGetProgramiv, shaderProgram, GL_LINK_STATUS);

    /* Fix the texture mapping unit in our shader, and 
     * keep the index of the "zoom" uniform to use when rendering */

    glUseProgram(shaderProgram);
    CHECK_GL_ERROR;
    int loc = glGetUniformLocation(shaderProgram, "texture1");
    CHECK_GL_ERROR;
    glUniform1i(loc, 0);
    CHECK_GL_ERROR;
    monitorZoomLoc = glGetUniformLocation(shaderProgram, "zoom");
    CHECK_GL_ERROR;

    /* Build the vertex array object for the "monitor" - a rectangle that fills the viewport. 
     * Through OpenGL bindings this references a vertex buffer object and an element buffer object,
     * but we don't need to keep references to them.
     * 
     * The rectangle is static, and to cope with the changing window size, we reset the view port
     * on every frame. This also allows us to keep the aspect ratio correct.
     * 
     * If we put this "monitor" rectangle onto a 3D object like a virtual AKF60, we'd need to change
     * that approach to avoid distorting the rest of the scene, but it'll work fine for now.
     */

    GLuint vbo, ebo;
    glGenVertexArrays(1, &monitorVao);
    glBindVertexArray(monitorVao);

    // A rectangle that fills the viewport:
    // * four points, clockwise from top-right,
    // * x,y,z coordinates
    // * r,g,b colour components
    // * u,v texture coordinates
    float monitorVertices[] = {
        1.0f, 1.0f, 0.0f,    1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
        1.0f, -1.0f, 0.0f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,   1.0f, 1.0f, 0.0f,   0.0f, 1.0f};
    unsigned int monitorIndices[] = {0,1,3, 1,2,3};

    glGenBuffers(1, &vbo); 
    CHECK_GL_ERROR;
    glBindBuffer(GL_ARRAY_BUFFER, vbo); 
    CHECK_GL_ERROR;
    glBufferData(GL_ARRAY_BUFFER, sizeof(monitorVertices), monitorVertices, GL_STATIC_DRAW);
    CHECK_GL_ERROR;

    glGenBuffers(1, &ebo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(monitorIndices), monitorIndices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    /* Create the texture object to which we'll stream the Archimedes display */

    glGenTextures(1, &screenTexture);
    glBindTexture(GL_TEXTURE_2D, screenTexture);
    /* This 2048×1024 is the screen memory hardwired into the VIDC emulation */
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2048, 1024, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
    CHECK_GL_ERROR;
    /* Sometimes repeating the pattern is useful to see when we mess something up */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    CHECK_GL_ERROR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    CHECK_GL_ERROR;
    /* The default is to assume our textures have mipmaps, so must turn that off  */
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    CHECK_GL_ERROR;
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    CHECK_GL_ERROR;

    return SDL_TRUE;
}

// FIXME: Not actually used yet
void video_renderer_close()
{
    if (sdl_main_window)
    {
        SDL_DestroyWindow(sdl_main_window);
        sdl_main_window = NULL;
    }
    if (context)
    {
        SDL_GL_DeleteContext(context);
        context = NULL;
    }
}

/*Update display texture from memory bitmap src.*/
void video_renderer_update(BITMAP *src, int src_x, int src_y, int dest_x, int dest_y, int w, int h)
{
    if (skip_video_render) {
        return;
    }
    LOG_VIDEO_FRAMES("video_renderer_update: src=%i,%i dest=%i,%i size=%i,%i\n", src_x, src_y, dest_x, dest_y, w, h);

    /* Not sure why the VIDC emulation sends us negative widths etc. that we need to cope with 
     * but will leave this code here until I can understand it.
     */
    SDL_Rect texture_rect;

    texture_rect.x = dest_x;
    texture_rect.y = dest_y;
    texture_rect.w = w;
    texture_rect.h = h;

    if (src_x < 0)
    {
        texture_rect.w += src_x;
        src_x = 0;
    }
    if (src_x > 2047)
        return;
    if ((src_x + texture_rect.w) > 2047)
        texture_rect.w = 2048 - src_x;

    if (src_y < 0)
    {
        texture_rect.h += src_y;
        src_y = 0;
    }
    if (src_y > 2047)
        return;
    if ((src_y + texture_rect.h) > 2047)
        texture_rect.h = 2048 - src_y;

    if (texture_rect.x < 0)
    {
        texture_rect.w += texture_rect.x;
        texture_rect.x = 0;
    }
    if (texture_rect.x > 2047)
        return;
    if ((texture_rect.x + texture_rect.w) > 2047)
        texture_rect.w = 2048 - texture_rect.x;

    if (texture_rect.y < 0)
    {
        texture_rect.h += texture_rect.y;
        texture_rect.y = 0;
    }
    if (texture_rect.y > 2047)
        return;
    if ((texture_rect.y + texture_rect.h) > 2047)
        texture_rect.h = 2048 - texture_rect.y;

    if (texture_rect.w <= 0 || texture_rect.h <= 0)
        return;

    LOG_VIDEO_FRAMES("SDL_UpdateTexture (%d, %d)+(%d, %d) from src (%d, %d) w %d\n",
                     texture_rect.x, texture_rect.y,
                     texture_rect.w, texture_rect.h,
                     src_x, src_y, src->w);

    /* We continually update a single texture because that 's the old 
    *  interface from the VIDC, the data is just pushed at us.  It might be
    *  faster to use 2 or 3 textures and swap them, but don't know yet.
    */

    glBindTexture(GL_TEXTURE_2D, screenTexture);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, src->w);
    CHECK_GL_ERROR;
    glTexSubImage2D(
        GL_TEXTURE_2D, 0,
        texture_rect.x, texture_rect.y, texture_rect.w, texture_rect.h,
        /* it's BGRA really but OpenGL ES won't do that - so the fragment shader swaps it */
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        src->dat + (src_y * src->w * 4) + src_x * 4);
    CHECK_GL_ERROR;
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    CHECK_GL_ERROR;

#ifdef __EMSCRIPTEN__
	if (take_screenshot || record_video) {
		take_screenshot = 0;
		SDL_Rect window_rect;
		SDL_GetWindowSize(sdl_main_window, &window_rect.w, &window_rect.h);
		EM_ASM({
			capture_frame($0,$1,$2,$3,$4,$5,$6,$7,$8);
		}, src->dat, src->w, src->h, src_x, src_y, texture_rect.w, texture_rect.h, window_rect.w, window_rect.h);
	}
#endif

    /* I'm not sure why this function is separate from video_renferer_present; 
     * as far as I can tell they are always called together 
     */
}

/*Render display texture to video window.*/
void video_renderer_present(int src_x, int src_y, int src_w, int src_h, int dblscan)
{
	if (skip_video_render)
		return;

    LOG_VIDEO_FRAMES("video_renderer_present: %d,%d + %d,%d\n", src_x, src_y, src_w, src_h);

    /* Adjust viewport so we display 4:3 as best we can in the window */

    SDL_Rect window, viewport;
	SDL_GetWindowSize(sdl_main_window, &window.w, &window.h);
    viewport = window;
    float aspect = window.w / window.h;
    if (aspect > 4.0f/3.0f) {
        viewport.w = viewport.h * 4.0f / 3.0f;
    } else if (aspect < 4.0f/3.0f) {
        viewport.h = viewport.w * 3.0f / 4.0f;
    }
    viewport.x = (window.w - viewport.w) / 2;
    viewport.y = (window.h - viewport.h) / 2;
    glViewport(viewport.x, viewport.y, viewport.w, viewport.h);
    CHECK_GL_ERROR;

    /* Set the zoom parameters in the fragment shader, 
     * so we only see the bit of the video memory that 
     * the VIDC asks for */

    glUniform4f(monitorZoomLoc, src_x, src_y, src_w, src_h);
    CHECK_GL_ERROR;

    glClearColor(0.0f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    /* Draw the monitor */

    glActiveTexture(GL_TEXTURE0); CHECK_GL_ERROR;
    glBindTexture(GL_TEXTURE_2D, screenTexture); CHECK_GL_ERROR;
    glUseProgram(shaderProgram); CHECK_GL_ERROR;
    glBindVertexArray(monitorVao); CHECK_GL_ERROR;
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0); CHECK_GL_ERROR;

    /* Should we handle framebuffers a bit more explicitly? This seems to work. */

    SDL_GL_SwapWindow(sdl_main_window);
}
