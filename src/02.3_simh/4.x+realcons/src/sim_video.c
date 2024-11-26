/* sim_video.c: Bitmap video output

Copyright (c) 2011-2013, Matt Burke
Copyright (c) 2018, Ian Schofield

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
the rights to use, copy, modify, merge, publish, distribute, sublicense,
and/or sell copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
THE AUTHOR BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of the author shall not be
used in advertising or otherwise to promote the sale, use or other dealings
in this Software without prior written authorization from the author.

08-Nov-2013  MB      Added globals for current mouse status
11-Jun-2013  MB      First version
01-Jan-2018  ISS     Major changes to the structure of the video system. Functionality unchanged.
                     This module is now used by all simulators requiring display services.
                     SDL (Simple Direct Media Layer) version 2.x is now the default as this
                     is supported by many operating systems.

*/

#include "sim_defs.h"
#include "sim_video.h"
#include "scp.h"
#include "display/display.h"
#if defined(HAVE_LIBPNG)
#include <png.h>
#endif
// Global video system flgs/variables

uint32 vid_mono_palette[2];                         /* Monochrome Color Map */
double *colmap;										/* Parameter used by Refresh and vid_setpixel in sim_video.c */
int32 pxval;										/* Also referenced in display.c */
int nostore=0;							            /* Enables storage display. Always defined. */
int alias;                                          /* Aliasing active flag. Required to set correct pixel intensity */

char vid_release_key[64] = "Ctrl-Right-Shift";


// Only include SDL code if specified. See end of this module for stubbed code.

#if defined(USE_SIM_VIDEO) && defined(HAVE_LIBSDL)

#define CUR_SIZE		5
#define EVENT_EXIT      8                              /* program exit */
#define EVENT_BEEP      10                              /* audio beep */


static int iwd,iht,told,tnew,tvl,dflag;
static const int MAXFONTPATH = 500;
static int init_x = 0;		// Initial window position and size
static int init_y = 0;
static unsigned int init_w = 0;
static unsigned int init_h = 0;
static int32 lcurr_x = 0;
static int32 lcurr_y = 0;
static char vid_title[128];
static int screen_num,xloc,yloc;
static uint32 lstst=0,lstcd=0;
static unsigned char *pixels;
static int pitch,surlen;
SIM_MOUSE_EVENT *xmev = 0,*xhev = 0;
SIM_KEY_EVENT *xkev = 0;
static enum vid_stat{STOPPED,WINDOW_OK,RUNNING,CLOSING,CLOSED}vid_init;
static int init_flags;
unsigned int pixelval;					// 24 bit RGB value
struct display *dp;
static int Refresh(void *info);
static int MLoop();
static void vid_close_window(void);
void vid_beep (void);
static void vid_beep_setup (int duration_ms, int tone_frequency);
static void vid_beep_cleanup (void);
static void vid_beep_event (void);

static SDL_Window *window = 0;                    // Declare some pointers
static SDL_Surface *surface = 0;
static SDL_Cursor *cursor = 0;
Uint32 render_flags = SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC;
SDL_Texture *tex;
	// creates a renderer to render our images
SDL_Renderer* rend;
int32 vid_flags;                                        /* Open Flags */

t_stat vid_set_release_key (FILE* st, UNIT* uptr, int32 val, CONST void* desc)
{
    return SCPE_NOFNC;
}

t_stat vid_show_release_key (FILE* st, UNIT* uptr, int32 val, CONST void* desc)
{
    if (vid_flags & SIM_VID_INPUTCAPTURED)
        fprintf (st, "ReleaseKey=%s", vid_release_key);
    else
        fprintf (st, "Not captured");
    return SCPE_OK;
}

t_stat vid_show_video (FILE* st, UNIT* uptr, int32 val, CONST void* desc)
{
int i;

fprintf (st, "Video support using SDL: %s\n", vid_version());
#if defined (SDL_MAIN_AVAILABLE)
fprintf (st, "  SDL Events being processed on the main process thread\n");
#endif
if (window) {
    fprintf (st, "  Currently Active Video Window: (%d by %d pixels)\n", init_w, init_h);
    fprintf (st, "  ");
    vid_show_release_key (st, uptr, val, desc);
    fprintf (st, "\n");
    }
for (i = 0; i < SDL_GetNumVideoDisplays(); ++i) {
    SDL_DisplayMode display;

    if (SDL_GetCurrentDisplayMode(i, &display)) {
        fprintf (st, "Could not get display mode for video display #%d: %s", i, SDL_GetError());
        }
    else {
        fprintf (st, "  Display %s(#%d): current display mode is %dx%dpx @ %dhz. \n", SDL_GetDisplayName(i), i, display.w, display.h, display.refresh_rate);
        }
    }
fprintf (st, "  Available SDL Renderers:\n");
for (i = 0; i < SDL_GetNumRenderDrivers(); ++i) {
    SDL_RendererInfo info;

    if (SDL_GetRenderDriverInfo (i, &info)) {
        fprintf (st, "Could not get render driver info for driver #%d: %s", i, SDL_GetError());
        }
    else {
        uint32 j, k;
        static struct {uint32 format; const char *name;} PixelFormats[] = {
            {SDL_PIXELFORMAT_INDEX1LSB,     "Index1LSB"},
            {SDL_PIXELFORMAT_INDEX1MSB,     "Index1MSB"},
            {SDL_PIXELFORMAT_INDEX4LSB,     "Index4LSB"},
            {SDL_PIXELFORMAT_INDEX4MSB,     "Index4MSB"},
            {SDL_PIXELFORMAT_INDEX8,        "Index8"},
            {SDL_PIXELFORMAT_RGB332,        "RGB332"},
            {SDL_PIXELFORMAT_RGB444,        "RGB444"},
            {SDL_PIXELFORMAT_RGB555,        "RGB555"},
            {SDL_PIXELFORMAT_BGR555,        "BGR555"},
            {SDL_PIXELFORMAT_ARGB4444,      "ARGB4444"},
            {SDL_PIXELFORMAT_RGBA4444,      "RGBA4444"},
            {SDL_PIXELFORMAT_ABGR4444,      "ABGR4444"},
            {SDL_PIXELFORMAT_BGRA4444,      "BGRA4444"},
            {SDL_PIXELFORMAT_ARGB1555,      "ARGB1555"},
            {SDL_PIXELFORMAT_RGBA5551,      "RGBA5551"},
            {SDL_PIXELFORMAT_ABGR1555,      "ABGR1555"},
            {SDL_PIXELFORMAT_BGRA5551,      "BGRA5551"},
            {SDL_PIXELFORMAT_RGB565,        "RGB565"},
            {SDL_PIXELFORMAT_BGR565,        "BGR565"},
            {SDL_PIXELFORMAT_RGB24,         "RGB24"},
            {SDL_PIXELFORMAT_BGR24,         "BGR24"},
            {SDL_PIXELFORMAT_RGB888,        "RGB888"},
            {SDL_PIXELFORMAT_RGBX8888,      "RGBX8888"},
            {SDL_PIXELFORMAT_BGR888,        "BGR888"},
            {SDL_PIXELFORMAT_BGRX8888,      "BGRX8888"},
            {SDL_PIXELFORMAT_ARGB8888,      "ARGB8888"},
            {SDL_PIXELFORMAT_RGBA8888,      "RGBA8888"},
            {SDL_PIXELFORMAT_ABGR8888,      "ABGR8888"},
            {SDL_PIXELFORMAT_BGRA8888,      "BGRA8888"},
            {SDL_PIXELFORMAT_ARGB2101010,   "ARGB2101010"},
            {SDL_PIXELFORMAT_YV12,          "YV12"},
            {SDL_PIXELFORMAT_IYUV,          "IYUV"},
            {SDL_PIXELFORMAT_YUY2,          "YUY2"},
            {SDL_PIXELFORMAT_UYVY,          "UYVY"},
            {SDL_PIXELFORMAT_YVYU,          "YVYU"},
            {SDL_PIXELFORMAT_UNKNOWN,       "Unknown"}};

        fprintf (st, "     Render #%d - %s\n", i, info.name);
        fprintf (st, "        Flags: 0x%X - ", info.flags);
        if (info.flags & SDL_RENDERER_SOFTWARE)
            fprintf (st, "Software|");
        if (info.flags & SDL_RENDERER_ACCELERATED)
            fprintf (st, "Accelerated|");
        if (info.flags & SDL_RENDERER_PRESENTVSYNC)
            fprintf (st, "PresentVSync|");
        if (info.flags & SDL_RENDERER_TARGETTEXTURE)
            fprintf (st, "TargetTexture|");
        fprintf (st, "\n");
        if ((info.max_texture_height != 0) || (info.max_texture_width != 0))
            fprintf (st, "        Max Texture: %d by %d\n", info.max_texture_height, info.max_texture_width);
        fprintf (st, "        Pixel Formats:\n");
        for (j=0; j<info.num_texture_formats; j++) {
            for (k=0; 1; k++) {
                if (PixelFormats[k].format == info.texture_formats[j]) {
                    fprintf (st, "            %s\n", PixelFormats[k].name);
                    break;
                    }
                if (PixelFormats[k].format == SDL_PIXELFORMAT_UNKNOWN) {
                    fprintf (st, "            %s - 0x%X\n", PixelFormats[k].name, info.texture_formats[j]);
                    break;
                    }
                }
            }
        }
    }

if (window)
    fprintf (st, "  Currently Active Renderer: System default\n");

if (1) {
    static const char *hints[] = {
#if defined (SDL_HINT_FRAMEBUFFER_ACCELERATION)
                SDL_HINT_FRAMEBUFFER_ACCELERATION   ,
#endif
#if defined (SDL_HINT_RENDER_DRIVER)
                SDL_HINT_RENDER_DRIVER              ,
#endif
#if defined (SDL_HINT_RENDER_OPENGL_SHADERS)
                SDL_HINT_RENDER_OPENGL_SHADERS      ,
#endif
#if defined (SDL_HINT_RENDER_DIRECT3D_THREADSAFE)
                SDL_HINT_RENDER_DIRECT3D_THREADSAFE ,
#endif
#if defined (SDL_HINT_RENDER_DIRECT3D11_DEBUG)
                SDL_HINT_RENDER_DIRECT3D11_DEBUG    ,
#endif
#if defined (SDL_HINT_RENDER_SCALE_QUALITY)
                SDL_HINT_RENDER_SCALE_QUALITY       ,
#endif
#if defined (SDL_HINT_RENDER_VSYNC)
                SDL_HINT_RENDER_VSYNC               ,
#endif
#if defined (SDL_HINT_VIDEO_ALLOW_SCREENSAVER)
                SDL_HINT_VIDEO_ALLOW_SCREENSAVER    ,
#endif
#if defined (SDL_HINT_VIDEO_X11_XVIDMODE)
                SDL_HINT_VIDEO_X11_XVIDMODE         ,
#endif
#if defined (SDL_HINT_VIDEO_X11_XINERAMA)
                SDL_HINT_VIDEO_X11_XINERAMA         ,
#endif
#if defined (SDL_HINT_VIDEO_X11_XRANDR)
                SDL_HINT_VIDEO_X11_XRANDR           ,
#endif
#if defined (SDL_HINT_GRAB_KEYBOARD)
                SDL_HINT_GRAB_KEYBOARD              ,
#endif
#if defined (SDL_HINT_MOUSE_RELATIVE_MODE_WARP)
                SDL_HINT_MOUSE_RELATIVE_MODE_WARP    ,
#endif
#if defined (SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS)
                SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS   ,
#endif
#if defined (SDL_HINT_IDLE_TIMER_DISABLED)
                SDL_HINT_IDLE_TIMER_DISABLED ,
#endif
#if defined (SDL_HINT_ORIENTATIONS)
                SDL_HINT_ORIENTATIONS ,
#endif
#if defined (SDL_HINT_ACCELEROMETER_AS_JOYSTICK)
                SDL_HINT_ACCELEROMETER_AS_JOYSTICK ,
#endif
#if defined (SDL_HINT_XINPUT_ENABLED)
                SDL_HINT_XINPUT_ENABLED ,
#endif
#if defined (SDL_HINT_GAMECONTROLLERCONFIG)
                SDL_HINT_GAMECONTROLLERCONFIG ,
#endif
#if defined (SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS)
                SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS ,
#endif
#if defined (SDL_HINT_ALLOW_TOPMOST)
                SDL_HINT_ALLOW_TOPMOST ,
#endif
#if defined (SDL_HINT_TIMER_RESOLUTION)
                SDL_HINT_TIMER_RESOLUTION ,
#endif
#if defined (SDL_HINT_VIDEO_HIGHDPI_DISABLED)
                SDL_HINT_VIDEO_HIGHDPI_DISABLED ,
#endif
#if defined (SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK)
                SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK ,
#endif
#if defined (SDL_HINT_VIDEO_WIN_D3DCOMPILER)
                SDL_HINT_VIDEO_WIN_D3DCOMPILER              ,
#endif
#if defined (SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT)
                SDL_HINT_VIDEO_WINDOW_SHARE_PIXEL_FORMAT    ,
#endif
#if defined (SDL_HINT_WINRT_PRIVACY_POLICY_URL)
                SDL_HINT_WINRT_PRIVACY_POLICY_URL ,
#endif
#if defined (SDL_HINT_WINRT_PRIVACY_POLICY_LABEL)
                SDL_HINT_WINRT_PRIVACY_POLICY_LABEL ,
#endif
#if defined (SDL_HINT_WINRT_HANDLE_BACK_BUTTON)
                SDL_HINT_WINRT_HANDLE_BACK_BUTTON ,
#endif
#if defined (SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES)
                SDL_HINT_VIDEO_MAC_FULLSCREEN_SPACES,
#endif
                NULL};
    fprintf (st, "  Currently Active SDL Hints:\n");
    for (i=0; hints[i]; i++) {
        if (SDL_GetHint (hints[i]))
            fprintf (st, "      %s = %s\n", hints[i], SDL_GetHint (hints[i]));
        }
    }
return SCPE_OK;
}

t_stat vid_show (FILE* st, DEVICE *dptr,  UNIT* uptr, int32 val, CONST char* desc)
{
    return vid_show_video (st, uptr, val, desc);
}


/*	This section ensures that the window creation and message loop run on the main thread.
This is specifically required by OSX die to the requirement of the Cocoa graphics system.
The method applied here is derived from the SDL specification.
*/

#if defined (main)
#undef main
#endif

static int main_argc;
static char **main_argv;
static SDL_Thread *vid_main_thread_handle;

int main_thread (void *arg)
{
    int stat;

    stat = SDL_main (main_argc, main_argv);	// Actually calls the main(...) function in scp.c
    vid_init = CLOSING;								// Will close MLoop
    return stat;
}

// THis is the entry point called if the graphicsc system is enable.

int main (int argc, char *argv[])
{
    int status;

    main_argc = argc;
    main_argv = argv;


    vid_main_thread_handle = SDL_CreateThread (main_thread , "simh-main", NULL);

    sim_os_set_thread_priority (PRIORITY_ABOVE_NORMAL);

    while (!MLoop())
        SDL_Delay(1);								// Transiently de-schedule this thread.

    // SDL_WaitThread (vid_main_thread_handle, &status);
    vid_close();
    SDL_Quit();
    return status;
}

t_stat vid_close (void){

    // Close and destroy the window if open
    if (vid_init != RUNNING)
        return 0;
    if (cursor)
        SDL_FreeCursor(cursor);
    vid_beep_cleanup();
    vid_init = CLOSED;
    while (vid_init != STOPPED)
        SDL_Delay(100);			// Wait for MLoop to complete window close.

    // Clean up
    cursor = 0;
    nostore = 0;
    surface = 0;
    if (xmev) {
        free(xmev);
        free(xhev);
        free(xkev);
    }
	display_reset();
    return 0;
}


/* Create a window that will report various events on the main thread ... required for OSX. */
/* For the QVSS display, set nostore such that the window will merely be refreshed */
/* Also, if QVSS, pixel data arrives from vax_vc.c already formatted. Using vid_draw instead. */
/* In the event that this function is called with a NULL title, storage mode (no fade) is assumed */
/* vid_init states as follows:
STOPPED. Initial state, no window, no message loop.
WINDOW_OK. Set by vid_open. MLoop will then call vid_create_window on main thread. -> RUNNING
RUNNING. Running state. Window open, message loop active.
CLOSING. Set by exit from main simh thread ... inititate shutdown. Exit from MLoop in main.
CLOSED. Set by vid_close. MLoop will close the window and move to state STOPPED.

At present init_flags may only contain SIM_VID_INPUTCATUED or SIM_OWN_CURSOR.

Comment: Some if the above could have been achieved using the SDL messaging system.
*/


t_stat vid_open (DEVICE *dptr, const char *title, uint32 width, uint32 height, int flags)
{
    init_w = width;
    init_h = height;
    init_flags = flags;
    if ((strlen(sim_name) + 7 + (dptr ? strlen (dptr->name) : 0) + (title ? strlen (title) : 0)) < sizeof (vid_title))
        sprintf (vid_title, "%s%s%s%s%s", sim_name, dptr ? " - " : "", dptr ? dptr->name : "", title ? " - " : "", title ? title : "");
    else
        sprintf (vid_title, "%s", sim_name);

    if (init_flags & SIM_VID_INPUTCAPTURED)
        strcat(vid_title, "               Captured input mode: ReleaseKey is Right-Ctrl+Right-Shift");


    if (!title) 
        nostore++;

    vid_mono_palette[0] = sim_end ? 0xFF000000 : 0x000000FF;        /* Black */
    vid_mono_palette[1] = 0xFFFFFFFF;                               /* White */
    vid_beep_setup(100, 660);
    xmev=(struct mouse_event *)calloc(1,sizeof(SIM_MOUSE_EVENT));
    xhev=(struct mouse_event *)calloc(1,sizeof(SIM_MOUSE_EVENT));
    xkev=(struct key_event *)calloc(1,sizeof(SIM_KEY_EVENT));

    vid_init = WINDOW_OK;
    alias = 0;				/* Anti-aliasing off */										    // Flag such that MLoop can call vid_create_window 
    return 0;
}

/*	This function creates the actual window and is called from MLoop on the main thread.
The expcted pixel format is 4x8 bytes. Any other format will result in an error exit.
This may seem rather limiting. However, modern hardware generally provides this format.
In the event that errors are reported, this function wil be updated.
NB. This function creates a renderer which is not used. The display system directly
manipulates the pixels in the surface buffer. This buffer is then written to the screen in Refresh().
*/


t_stat vid_create_window(void)
{
    // Create an application window with the following settings:
	// Use vt11_window set set window size
	//printf("WIN:%d\r\n",vt11_window);
	//vt11_window=WIN_NORM;
    SDL_Init (SDL_INIT_VIDEO);
    SDL_SetHint(SDL_HINT_RENDER_VSYNC,"1");
    window = SDL_CreateWindow(
        vid_title,							// window title
        SDL_WINDOWPOS_UNDEFINED,					// initial x position
        SDL_WINDOWPOS_UNDEFINED,					// initial y position
        init_w,                             				// width, in pixels
        init_h,                             				// height, in pixels
        (vt11_window==WIN_FULL)?SDL_WINDOW_FULLSCREEN:SDL_WINDOW_SHOWN						// flags
        );

    // Check that the window was successfully created
    if (window == NULL) {
        // In the case that the window could not be made...
        printf("Could not create SDL window: %s\n", SDL_GetError());
        exit(-1);
    }
    
    surface = SDL_CreateRGBSurfaceWithFormat(0, init_w, init_h, 32, SDL_PIXELFORMAT_RGB888);
    /* Check the bitdepth of the surface */
    if(surface->format->BitsPerPixel != 32){
        fprintf(stderr, "Not an 32-bit SDL surface.\n");
        exit(-1);
    }
    /* Check the byte count of the surface */
    if(surface->format->BytesPerPixel != 4){
        fprintf(stderr, "Invalid pixel format.\n");
        exit(-1);
    }
    
    rend = SDL_GetRenderer(window);
    if (rend)
        SDL_DestroyRenderer(rend);
    rend=SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (!rend)
        printf("%s\r\n",SDL_GetError());
    tex = SDL_CreateTextureFromSurface(rend, surface);
    if (!tex)
        printf("%s\r\n",SDL_GetError());

    pixels = (unsigned char *)surface->pixels;
    surlen = (init_h * surface->pitch);
    if (!(init_flags & SIM_VID_INPUTCAPTURED))
        if (!(init_flags & SIM_OWN_CURSOR))
             SDL_ShowCursor(SDL_DISABLE);			        /* Make host OS cursor invisible in non-capture mode or */
                                                            /* for all other system that use their own cursor (see sim_ws.c:vid_open) */
    vid_init = RUNNING;									    /* Init OK continue to next state */
    dflag = 0;
    return SCPE_OK;
}

#if defined(HAVE_LIBPNG)

/*
This function coverts the default ARGB surface pixel format to ABGR (RGBA) which is the
only 32 bit format that is managed by libpng. Not very tidy!
Also, set A to 0xff.
*/

t_stat write_png_file(char *filename) {
    int i;
    Uint8 *p,*buffer,tm;
    png_bytep *row_pointers;
    Uint8 *pixels;

    FILE *fp = fopen(filename, "wb");

    png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

    png_infop info = png_create_info_struct(png);

    png_init_io(png, fp);

    buffer = (Uint8 *)malloc(surlen);
    pixels = (Uint8 *)surface->pixels;
    memcpy(buffer, pixels, surlen);
    for (i=0,p=buffer;i<surlen/4;i++,p+=4) {
        tm = p[0];
        p[0] = p[2];
        p[2] = tm;
        p[3] = 0xff;
    }

    row_pointers = (png_bytep *)malloc(sizeof(png_bytep)*init_h);
    for (i = 0; i < surface->h; i++)
        row_pointers[i] = (png_bytep)(Uint8 *)buffer + i * surface->pitch;

    // Output is 8bit depth, RGBA format.

    png_set_IHDR(
        png,
        info,
        init_w, init_h,
        8,
        PNG_COLOR_TYPE_RGBA,
        PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_DEFAULT,
        PNG_FILTER_TYPE_DEFAULT
        );
    png_write_info(png, info);

    png_write_image(png, row_pointers);
    png_write_end(png, NULL);

    free(row_pointers);

    fclose(fp);
    return 0;
}
#else
t_stat write_png_file(char *filename) {
    printf("PNG library not available\n");
    return 1;								/* Always return an error */
}
#endif

t_stat vid_screenshot (const char *filename)
{
    int stat;
    char *fullname = NULL;

    if (!surface) {
        sim_printf ("No video display is active\n");
        return SCPE_UDIS | SCPE_NOMESSAGE;
    }
    fullname = (char *)malloc (strlen(filename) + 5);
    if (!filename)
        return SCPE_MEM;
    if (1) {
#if defined(HAVE_LIBPNG)
        if (!match_ext (filename, "bmp")) {
            sprintf (fullname, "%s%s", filename, match_ext (filename, "png") ? "" : ".png");
            stat = write_png_file(fullname);
        }
        else {
            sprintf (fullname, "%s", filename);
            stat = SDL_SaveBMP(surface, fullname);
        }
#else
        sprintf (fullname, "%s%s", filename, match_ext (filename, "bmp") ? "" : ".bmp");
        stat = SDL_SaveBMP(surface, fullname);
#endif /* defined(HAVE_LIBPNG) */
    }
    if (stat) {
        sim_printf ("Error saving screenshot to %s: %s\n", fullname, SDL_GetError());
        free (fullname);
        return SCPE_IOERR | SCPE_NOMESSAGE;
    }
    else {
        if (!sim_quiet)
            sim_printf ("Screenshot saved to %s\n", fullname);
        free (fullname);
        return SCPE_OK;
    }
}

void vid_set_cursor_position (int32 x, int32 y) {

    lcurr_x = x;
    lcurr_y = y;
}


t_stat vid_set_cursor (t_bool visible, uint32 width, uint32 height, uint8 *data, uint8 *mask, uint32 hot_x, uint32 hot_y)
{

    if (!cursor) {
        while (vid_init != RUNNING)         /* Wait for Window to be created */
            SDL_Delay(10);
        cursor = SDL_CreateCursor (data, mask, width, height, hot_x, hot_y);
        SDL_SetCursor(cursor);
        SDL_ShowCursor(SDL_ENABLE);			/* Make new cursor visible */
        return SCPE_OK;
    }
    return SCPE_NOFNC;
}


t_stat vid_erase_win()
{
    memset(surface->pixels, 0, init_w * init_h * 4);
    SDL_UpdateWindowSurface(window);				/* Write black surface to the host system window */
    return 0;
}

/*	
vid_draw. Similar to bitblt (Win.GDI) simply copies a pre-formatted buffer into the current surface.
The surface is then copied to the screen every 20 mS. in Refresh() as below.
*/

void vid_draw (int32 x, int32 y, int32 w, int32 h, uint32 *buf)
{

    int32 i;
    uint32* pixels;

    if (vid_init != RUNNING)
        return;

    pixels = (uint32 *)surface->pixels;
    for (i = 0; i < h; i++)
        memcpy (pixels + ((i + y) * init_w) + x, buf + w*i, w*sizeof(*pixels));

    return;
}

/*
The function is central to all display system. Firstly, the current surface (effectively a back buffer)
is copied to the screen surface by SDL_UpdateWindowSurface. This function (hopefully) calls a GPU
bitblt process and with modern hardware is extremely fast (1 mS.).
The next section su responsible for gradually fading the image in the window. This is achieved using an RGB time
constant sequence in colmap such that the phosphor fade constant for each color can be different. This loop is also very fast
typically 10-15 mS. on recent X86 family processors clocked at >1.5 GHz. The final part of the loop ensure that the
entire cycle takes almost 20 mS. with due allowance for scheduling  etc. In this case, the fade constants can be preset and
no matter what is on screen, the fading will be constant.
As the fade constants are set by color, some interesting effects can be obtained. See display.c for examples where the pixels
can start of as blue/white and then have a yellow fade effect. Eg P40 used in the VS60 system.
Howvwer, for some displays eg QVSS, a fade is not required so this function is disabled by nostore. In this case, the system
emeulates a standard RGB bitmap display in framebuffer mode.
The nostore function can also be used for a storage scope display eg the Tektronix 611 XY display. (See pdp8_vc8.c).
*/

static int Refresh(void *info) {
    int i,j;
    unsigned char *p;
    double *d;
    
    if (window && vid_init != STOPPED) {

        if (vid_init == RUNNING) {				            // If halted ... freeze display and, display valid
            if (!nostore && sim_is_running)                 // Only decay the pixels in store mode and if the simulator is running
                for (i = 0,p=(unsigned char *)pixels;i < surlen/4; i++, p++)
                    for (j = 0,d = colmap;j < 3;j++, p++, d++)
                        if (*p)
                            *p = (unsigned char)((double)*p * (*d));	// Decay pixels>0 only
        }
    }
    return 0;							// The window has been closed by vid_close ... exit thread
}

/*	The Mouse system.

The following function is central to the mouse implementation for the QVSS (VCB01) Q-bus card.
The intention of this function is to provide seamless enrty and exit of the mouse pointer
from within the host window system. The first 6 lines of code relate to window initialisation.
This could equally have been achived using messages but a state machine is more convenient.
The next 6 lines constitute a system that generates the required relative motion data so that the
QVSS mouse location generated by the OS ruunning on the simh client eg VMS is matched to the
location of the (actaully invisible) cursor managed by the host OS. The is achived by calculating
the X and Y pointing errors and generating a sequence of relative movements until the location of
the client cursor matches that of the host. The conditional expression reduces the 'gain' of this
servo loop for relative errors > 8. This is required because the client OS response to a given
relative move is non-linear due to a degree of mouse acceleration implemented by the client. THis
non linear response is implemented in both DEC Windows under VMS and Ultrix.
The overall efect of this process is that when the host cursor enter the QVSS window, the client
OS is sent a number of move messages to move the client cursor to the same location as the
now invisible host cursor.
This interface is also used by all systems with a light pen (VT11/VT60/PDP1/TX-0).
Similarly, the keyboard data is made available to all systems (QVSS only at present).

*/

static void vid_close_window(void)
{
    stop_cpu = 1;								// Stop simulation
    vid_init = STOPPED;							// Reset vid_init state
    SDL_DestroyWindow(window);					// Close this window and renderer.
    window = 0;									// Invalidate handle
}

static int MLoop() {
    SDL_Event event;
    int rel_x,rel_y;

    switch (vid_init) {
        case STOPPED:
            return 0;				// Wait until window has been initialised
        case WINDOW_OK:
            vid_create_window();	// Create window and begin receiving events
            return 0;
        case RUNNING:               // Update display
        if (dflag++<8)
            break;
		//SDL_UpdateWindowSurface(window); 
		SDL_UpdateTexture(tex, NULL, surface->pixels, surface->pitch);
		//SDL_RenderClear(rend);
		SDL_RenderCopy(rend, tex, NULL, NULL);
		SDL_RenderPresent(rend);
        Refresh(NULL);
        dflag=0;
            break;
        case CLOSING:
            return -1;				// Exit message loop and start shutdown
        case CLOSED:
            vid_close_window();		// Close window continue receiving events until SDL_Quit
            break;
    }

    rel_x = xmev->x_pos - lcurr_x;
    rel_y = xmev->y_pos - lcurr_y;
    rel_x = (abs(rel_x) > 9)?rel_x/2:rel_x;
    rel_y = (abs(rel_y) > 9)?rel_y/2:rel_y;

    xmev->x_rel = rel_x;
    xmev->y_rel = rel_y;


    if (SDL_PollEvent(&event)) {
        switch (event.type) {
                case SDL_QUIT:
                    return 0;
                case SDL_MOUSEMOTION:
                    xmev->x_pos = event.motion.x-iwd;
                    xmev->y_pos = event.motion.y-iht;
                    break;
                case SDL_MOUSEBUTTONDOWN:
                    xmev->b1_state = (event.button.button==SDL_BUTTON_LEFT);
                    xmev->b2_state = (event.button.button==SDL_BUTTON_RIGHT);
                    xmev->b3_state = (event.button.button==SDL_BUTTON_MIDDLE);
                    xmev->x_pos = event.button.x-iwd;
                    xmev->y_pos = event.button.y-iht;
                    break;
                case SDL_MOUSEBUTTONUP:
                    if (event.button.button == SDL_BUTTON_LEFT) xmev->b1_state = 0;
                    if (event.button.button == SDL_BUTTON_RIGHT) xmev->b2_state = 0;
                    if (event.button.button == SDL_BUTTON_MIDDLE) xmev->b3_state = 0;
                    xmev->x_pos = event.button.x - iwd;
                    xmev->y_pos = event.button.y - iht;
                    break;
                case SDL_KEYDOWN:
                    xkev->key = event.key.keysym.sym;
                    xkev->state = SIM_KEYPRESS_DOWN;
                    xkev->mod = event.key.keysym.mod;
                    break;
                case SDL_KEYUP:
                    xkev->key = event.key.keysym.sym;
                    xkev->state = SIM_KEYPRESS_UP;
                    xkev->mod = event.key.keysym.mod;
                    break;
                case SDL_WINDOWEVENT:
                    if (event.window.event == SDL_WINDOWEVENT_CLOSE)
                        stop_cpu = 1;								// Stop simulation
                    break;
                case SDL_USEREVENT:
                    if (event.window.event == EVENT_BEEP)
                        vid_beep_event();
                    break;
                case SDL_TEXTINPUT:
                    write_console_input(event.text.text,1);
                    break;
                default:
                    return 0;
        }
    }
    return 0;
}

/* Only return SCPE_OK if there is a change in the mouse or keyboard data */

int vid_poll_mouse (SIM_MOUSE_EVENT *mev)
{

    if (!xmev)
        return SCPE_EOF;
    memcpy(mev,xmev,sizeof(SIM_MOUSE_EVENT));
    if ( !memcmp(xhev,xmev,sizeof(SIM_MOUSE_EVENT)) && !(mev->x_rel | xmev->y_rel) ) 
        return SCPE_EOF;							/* Only report changes */
    memcpy(xhev,xmev,sizeof(SIM_MOUSE_EVENT));		/* Keep local copy */

    return SCPE_OK;
}
t_stat vid_poll_kb (SIM_KEY_EVENT *ev) {

    uint32 kval;

    if (!xkev)
        return SCPE_EOF;
    if (xkev->state == lstst && lstcd == xkev->key)
        return SCPE_EOF;								/* Single events only */
    memcpy(ev,xkev,sizeof(SIM_KEY_EVENT));
    lstst = xkev->state;
    kval = lstcd = xkev->key;
    if (!xkev->state) {
        if (kval<32)
            write_console_input((char *)&kval,1);
        kval &= 31;
        if (xkev->mod & KMOD_CTRL)
            write_console_input((char *)&kval,1);
    }
    return SCPE_OK;
}

/*	
This function draws a pixel on the current surface if the graphics system is in state 2.
The surface is written to the screen (blit) in Refresh() above. ALL display systems use this
function. At present, level and color are ignored.
*/

t_stat vid_setpixel(int ix,int iy,int level,int color) {
    Uint32 *p,pmsk,i;
    unsigned char* pz,*pv=(unsigned char *)&pxval;

    pmsk = (alias << 16) | (alias << 8) | alias;
    if (vid_init == RUNNING) {
        p = (Uint32 *)(pixels + (iy * surface->pitch) + (ix * sizeof(Uint32)));
        pz = (pixels + (iy * surface->pitch) + (ix * sizeof(Uint32)));
        if (alias)
            for (i = 0; i < 3; i++, pz++ , pv++)
                *pz = (alias * *pv) >> 8;
        else
            *p = pxval;
    }
    return SCPE_OK;
}

t_stat vid_lock_cursor()
{
    SDL_ShowCursor(SDL_DISABLE);		   /* Make host OS cursor invisible in non-capture/default mode */
    SDL_SetWindowGrab(window, SDL_TRUE);   /* Lock mouse to this window */
#ifdef __APPLE__
    SDL_SetRelativeMouseMode(SDL_TRUE);
#endif
    return SCPE_OK;
}

t_stat vid_unlock_cursor()
{
    if (SDL_GetWindowGrab(window) == SDL_TRUE) {
        SDL_SetWindowGrab(window, SDL_FALSE);							/* Unlock mouse for this window */
        SDL_SetRelativeMouseMode(SDL_FALSE);
        SDL_ShowCursor(SDL_ENABLE);										/* Make host OS cursor visible in non-capture/default mode */
        SDL_WarpMouseInWindow(window, xmev->x_pos + 1, xmev->y_pos );	/* This is required to redraw the host cursor */
        return 1;
    }
    return 0;
}

const char *vid_version(void)
{
    static char SDLVersion[80];
    SDL_version compiled, running;

    SDL_GetVersion(&running);
    SDL_VERSION(&compiled);

    if ((compiled.major == running.major) &&
        (compiled.minor == running.minor) &&
        (compiled.patch == running.patch))
        sprintf(SDLVersion, "SDL Version %d.%d.%d", 
        compiled.major, compiled.minor, compiled.patch);
    else
        sprintf(SDLVersion, "SDL Version (Compiled: %d.%d.%d, Runtime: %d.%d.%d)", 
        compiled.major, compiled.minor, compiled.patch,
        running.major, running.minor, running.patch);
    return (const char *)SDLVersion;
}

/*	Original beep code from Matt Burke's code */


#include <SDL_audio.h>
#include <math.h>

const int AMPLITUDE = 20000;
const int SAMPLE_FREQUENCY = 11025;
static int16 *vid_beep_data;
static int vid_beep_offset;
static int vid_beep_duration;
static int vid_beep_samples;

static void vid_audio_callback(void *ctx, Uint8 *stream, int length)
{
    int16 *data = (int16 *)stream;
    int i, sum, remnant = ((vid_beep_samples - vid_beep_offset) * sizeof (*vid_beep_data));

    if (length > remnant) {
        memset (stream + remnant, 0, length - remnant);
        length = remnant;
        if (remnant == 0) {
            SDL_PauseAudio(1);
            return;
        }
    }
    memcpy (stream, &vid_beep_data[vid_beep_offset], length);
    for (i=sum=0; i<length; i++)
        sum += stream[i];
    vid_beep_offset += length / sizeof(*vid_beep_data);
}

static void vid_beep_setup (int duration_ms, int tone_frequency)
{
    if (!vid_beep_data) {
        int i;
        SDL_AudioSpec desiredSpec;

        memset (&desiredSpec, 0, sizeof(desiredSpec));
        desiredSpec.freq = SAMPLE_FREQUENCY;
        desiredSpec.format = AUDIO_S16SYS;
        desiredSpec.channels = 1;
        desiredSpec.samples = 2048;
        desiredSpec.callback = vid_audio_callback;

        SDL_OpenAudio(&desiredSpec, NULL);

        vid_beep_samples = (int)((SAMPLE_FREQUENCY * duration_ms) / 1000.0);
        vid_beep_duration = duration_ms;
        vid_beep_data = (int16 *)malloc (sizeof(*vid_beep_data) * vid_beep_samples);
        for (i=0; i<vid_beep_samples; i++)
            vid_beep_data[i] = (int16)(AMPLITUDE * sin(((double)(i * M_PI * tone_frequency)) / SAMPLE_FREQUENCY));
    }
}

static void vid_beep_cleanup (void)
{
    SDL_CloseAudio();
    free (vid_beep_data);
    vid_beep_data = NULL;
}

void vid_beep_event (void)
{
    vid_beep_offset = 0;                /* reset to beginning of sample set */
    SDL_PauseAudio (0);                 /* Play sound */
}

void vid_beep (void)
{
    SDL_Event user_event;

    user_event.type = SDL_USEREVENT;
    user_event.user.code = EVENT_BEEP;
    user_event.user.data1 = NULL;
    user_event.user.data2 = NULL;
#if defined (SDL_MAIN_AVAILABLE)
    while (SDL_PushEvent (&user_event) < 0)
        sim_os_ms_sleep (10);
#else
    vid_beep_event ();
#endif
    //	SDL_Delay (vid_beep_duration + 100);/* Wait for sound to finnish */
}


/*
    Translate SDL Keyboard codes to SIMH internal codes for use by any simulator
    requiring keyboard data from a window.
*/

int vid_map_key (int key)
{
switch (key) {

    case SDLK_BACKSPACE:
        return SIM_KEY_BACKSPACE;

    case SDLK_TAB:
        return SIM_KEY_TAB;

    case SDLK_RETURN:
        return SIM_KEY_ENTER;

    case SDLK_ESCAPE:
        return SIM_KEY_ESC;

    case SDLK_SPACE:
        return SIM_KEY_SPACE;

    case SDLK_QUOTE:
        return SIM_KEY_SINGLE_QUOTE;

    case SDLK_COMMA:
        return SIM_KEY_COMMA;

    case SDLK_MINUS:
        return SIM_KEY_MINUS;

    case SDLK_PERIOD:
        return SIM_KEY_PERIOD;

    case SDLK_SLASH:
        return SIM_KEY_SLASH;

    case SDLK_0:
        return SIM_KEY_0;

    case SDLK_1:
        return SIM_KEY_1;

    case SDLK_2:
        return SIM_KEY_2;

    case SDLK_3:
        return SIM_KEY_3;

    case SDLK_4:
        return SIM_KEY_4;

    case SDLK_5:
        return SIM_KEY_5;

    case SDLK_6:
        return SIM_KEY_6;

    case SDLK_7:
        return SIM_KEY_7;

    case SDLK_8:
        return SIM_KEY_8;

    case SDLK_9:
        return SIM_KEY_9;

    case SDLK_SEMICOLON:
        return SIM_KEY_SEMICOLON;

    case SDLK_EQUALS:
        return SIM_KEY_EQUALS;

    case SDLK_LEFTBRACKET:
        return SIM_KEY_LEFT_BRACKET;

    case SDLK_BACKSLASH:
        return SIM_KEY_BACKSLASH;

    case SDLK_RIGHTBRACKET:
        return SIM_KEY_RIGHT_BRACKET;

    case SDLK_BACKQUOTE:
        return SIM_KEY_BACKQUOTE;

    case SDLK_a:
        return SIM_KEY_A;

    case SDLK_b:
        return SIM_KEY_B;

    case SDLK_c:
        return SIM_KEY_C;

    case SDLK_d:
        return SIM_KEY_D;

    case SDLK_e:
        return SIM_KEY_E;

    case SDLK_f:
        return SIM_KEY_F;

    case SDLK_g:
        return SIM_KEY_G;

    case SDLK_h:
        return SIM_KEY_H;

    case SDLK_i:
        return SIM_KEY_I;

    case SDLK_j:
        return SIM_KEY_J;

    case SDLK_k:
        return SIM_KEY_K;

    case SDLK_l:
        return SIM_KEY_L;

    case SDLK_m:
        return SIM_KEY_M;

    case SDLK_n:
        return SIM_KEY_N;

    case SDLK_o:
        return SIM_KEY_O;

    case SDLK_p:
        return SIM_KEY_P;

    case SDLK_q:
        return SIM_KEY_Q;

    case SDLK_r:
        return SIM_KEY_R;

    case SDLK_s:
        return SIM_KEY_S;

    case SDLK_t:
        return SIM_KEY_T;

    case SDLK_u:
        return SIM_KEY_U;

    case SDLK_v:
        return SIM_KEY_V;

    case SDLK_w:
        return SIM_KEY_W;

    case SDLK_x:
        return SIM_KEY_X;

    case SDLK_y:
        return SIM_KEY_Y;

    case SDLK_z:
        return SIM_KEY_Z;

    case SDLK_DELETE:
        return SIM_KEY_DELETE;

    case SDLK_KP_0:
        return SIM_KEY_KP_INSERT;

    case SDLK_KP_1:
        return SIM_KEY_KP_END;

    case SDLK_KP_2:
        return SIM_KEY_KP_DOWN;

    case SDLK_KP_3:
        return SIM_KEY_KP_PAGE_DOWN;

    case SDLK_KP_4:
        return SIM_KEY_KP_LEFT;

    case SDLK_KP_5:
        return SIM_KEY_KP_5;

    case SDLK_KP_6:
        return SIM_KEY_KP_RIGHT;

    case SDLK_KP_7:
        return SIM_KEY_KP_HOME;

    case SDLK_KP_8:
        return SIM_KEY_KP_UP;

    case SDLK_KP_9:
        return SIM_KEY_KP_PAGE_UP;

    case SDLK_KP_PERIOD:
        return SIM_KEY_KP_DELETE;

    case SDLK_KP_DIVIDE:
        return SIM_KEY_KP_DIVIDE;

    case SDLK_KP_MULTIPLY:
        return SIM_KEY_KP_MULTIPLY;

    case SDLK_KP_MINUS:
        return SIM_KEY_KP_SUBTRACT;

    case SDLK_KP_PLUS:
        return SIM_KEY_KP_ADD;

    case SDLK_KP_ENTER:
        return SIM_KEY_KP_ENTER;

    case SDLK_UP:
        return SIM_KEY_UP;

    case SDLK_DOWN:
        return SIM_KEY_DOWN;

    case SDLK_RIGHT:
        return SIM_KEY_RIGHT;

    case SDLK_LEFT:
        return SIM_KEY_LEFT;

    case SDLK_INSERT:
        return SIM_KEY_INSERT;

    case SDLK_HOME:
        return SIM_KEY_HOME;

    case SDLK_END:
        return SIM_KEY_END;

    case SDLK_PAGEUP:
        return SIM_KEY_PAGE_UP;

    case SDLK_PAGEDOWN:
        return SIM_KEY_PAGE_DOWN;

    case SDLK_F1:
        return SIM_KEY_F1;

    case SDLK_F2:
        return SIM_KEY_F2;

    case SDLK_F3:
        return SIM_KEY_F3;

    case SDLK_F4:
        return SIM_KEY_F4;

    case SDLK_F5:
        return SIM_KEY_F5;

    case SDLK_F6:
        return SIM_KEY_F6;

    case SDLK_F7:
        return SIM_KEY_F7;

    case SDLK_F8:
        return SIM_KEY_F8;

    case SDLK_F9:
        return SIM_KEY_F9;

    case SDLK_F10:
        return SIM_KEY_F10;

    case SDLK_F11:
        return SIM_KEY_F11;

    case SDLK_F12:
        return SIM_KEY_F12;

    case SDLK_NUMLOCKCLEAR:
        return SIM_KEY_NUM_LOCK;

    case SDLK_CAPSLOCK:
        return SIM_KEY_CAPS_LOCK;

    case SDLK_SCROLLLOCK:
        return SIM_KEY_SCRL_LOCK;

    case SDLK_RSHIFT:
        return SIM_KEY_SHIFT_R;

    case SDLK_LSHIFT:
        return SIM_KEY_SHIFT_L;

    case SDLK_RCTRL:
        return SIM_KEY_CTRL_R;

    case SDLK_LCTRL:
        return SIM_KEY_CTRL_L;

    case SDLK_RALT:
        return SIM_KEY_ALT_R;

    case SDLK_LALT:
        return SIM_KEY_ALT_L;

    case SDLK_LGUI:
        return SIM_KEY_WIN_L;

    case SDLK_RGUI:
        return SIM_KEY_WIN_R;

    case SDLK_PRINTSCREEN:
        return SIM_KEY_PRINT;

    case SDLK_PAUSE:
        return SIM_KEY_PAUSE;

    case SDLK_MENU:
        return SIM_KEY_MENU;

    default:
        return SIM_KEY_UNKNOWN;
        }
}


#else /* !(defined(USE_SIM_VIDEO) && defined(HAVE_LIBSDL)) */
/* Non-implemented versions */

t_stat vid_open (DEVICE *dptr, const char *title, uint32 width, uint32 height, int flags)
{
    printf("SDL graphics services are not avaiable.\nTo use a graphics device, please verify the build environment.\n");
    return SCPE_NOFNC;
}

t_stat vid_close (void)
{
    return SCPE_OK;
}

t_stat vid_poll_kb (SIM_KEY_EVENT *ev)
{
    return SCPE_EOF;
}

t_stat vid_poll_mouse (SIM_MOUSE_EVENT *ev)
{
    return SCPE_EOF;
}

void vid_draw (int32 x, int32 y, int32 w, int32 h, uint32 *buf)
{
    return;
}

t_stat vid_erase_win(void)
{
    return SCPE_NOFNC;
}

t_stat vid_setpixel(ix, iy, level, color)
{
    return SCPE_NOFNC;
}

t_stat vid_set_cursor (t_bool visible, uint32 width, uint32 height, uint8 *data, uint8 *mask, uint32 hot_x, uint32 hot_y)
{
    return SCPE_NOFNC;
}

void vid_set_cursor_position (int32 x, int32 y)
{
    return;
}

void vid_refresh (void)
{
    return;
}

void vid_beep (void)
{
    return;
}

t_stat vid_lock_cursor()
{
    return SCPE_EOF;
}

t_stat vid_unlock_cursor()
{
    return SCPE_EOF;
}

int vid_map_key(int key)
{
    return 0;
}

t_stat vid_show_video (FILE* st, UNIT* uptr, int32 val, CONST void* desc)
{

    fprintf (st, "No video support. If required, please install SDL2\n");
    return SCPE_OK;
}

const char *vid_version (void)
{
    return "No Video Support";
}

t_stat vid_screenshot (const char *filename)
{
    sim_printf ("video support unavailable\n");
    return SCPE_NOFNC|SCPE_NOMESSAGE;
}
#endif /* defined(USE_SIM_VIDEO) */
