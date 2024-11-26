/*
 * simh sim_video support for XY display simulator
 * Mark Pizzolato <mark@infocomm.com>
 * January 2016
 * Based on win32.c module by Phil Budne
 */

/*
 * Copyright (c) 2016, Mark Pizzolato
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the names of the authors shall
 * not be used in advertising or otherwise to promote the sale, use or
 * other dealings in this Software without prior written authorization
 * from the authors.
 */

/*
 * BUGS:
 * Does not allow you to close display window;
 * would need to tear down both system, and system independent data.
 *
 */

/*
	Update by Dr Ian Schoield Dec 2017-
	See sim_video.c.
	A considerable amount of code has been removed as the
	display management process has changed.
*/

#include "sim_video.h"
#include <stdio.h>
#include <stdlib.h>
#include "ws.h"
#include "display.h"


#ifndef PIX_SIZE
#define PIX_SIZE 1
#endif

#ifndef PIX_SCALE
#define PIX_SCALE RES_FULL
#endif /* PIX_SCALE not defined */

#define Uint8 uint8

/*
 * light pen location
 * see ws.h for full description
 */
int ws_lp_x = -1;
int ws_lp_y = -1;
/*
 * light pen "tip switch" activated (for VS60 emulation etc.)
 * should only be set from "driver" (window system layer)
 */
unsigned char display_lp_sw;

/*
 * deactivates light pen
 * (SIMH DR11-C simulation when initialized sets this and
 * then reports mouse coordinates as Talos digitizer data)
 */
unsigned char display_tablet;

static int xpixels, ypixels;
static int pix_size = PIX_SCALE;
static const char *window_name;
static uint32 *colors = NULL;
static uint32 ncolors = 0, size_colors = 0;
typedef struct cursor {
    Uint8 *data;
    Uint8 *mask;
    int width;
    int height;
    int hot_x;
    int hot_y;
    } CURSOR;

static CURSOR *arrow_cursor;
static CURSOR *cross_cursor;


static  int
map_key(int k)
{
    switch (k) {
        case SIM_KEY_0:                   return '0';
        case SIM_KEY_1:                   return '1';
        case SIM_KEY_2:                   return '2';
        case SIM_KEY_3:                   return '3';
        case SIM_KEY_4:                   return '4';
        case SIM_KEY_5:                   return '5';
        case SIM_KEY_6:                   return '6';
        case SIM_KEY_7:                   return '7';
        case SIM_KEY_8:                   return '8';
        case SIM_KEY_9:                   return '9';
        case SIM_KEY_A:                   return 'a';
        case SIM_KEY_B:                   return 'b';
        case SIM_KEY_C:                   return 'c';
        case SIM_KEY_D:                   return 'd';
        case SIM_KEY_E:                   return 'e';
        case SIM_KEY_F:                   return 'f';
        case SIM_KEY_G:                   return 'g';
        case SIM_KEY_H:                   return 'h';
        case SIM_KEY_I:                   return 'i';
        case SIM_KEY_J:                   return 'j';
        case SIM_KEY_K:                   return 'k';
        case SIM_KEY_L:                   return 'l';
        case SIM_KEY_M:                   return 'm';
        case SIM_KEY_N:                   return 'n';
        case SIM_KEY_O:                   return 'o';
        case SIM_KEY_P:                   return 'p';
        case SIM_KEY_Q:                   return 'q';
        case SIM_KEY_R:                   return 'r';
        case SIM_KEY_S:                   return 's';
        case SIM_KEY_T:                   return 't';
        case SIM_KEY_U:                   return 'u';
        case SIM_KEY_V:                   return 'v';
        case SIM_KEY_W:                   return 'w';
        case SIM_KEY_X:                   return 'x';
        case SIM_KEY_Y:                   return 'y';
        case SIM_KEY_Z:                   return 'z';
        case SIM_KEY_BACKQUOTE:           return '`';
        case SIM_KEY_MINUS:               return '-';
        case SIM_KEY_EQUALS:              return '=';
        case SIM_KEY_LEFT_BRACKET:        return '[';
        case SIM_KEY_RIGHT_BRACKET:       return ']';
        case SIM_KEY_SEMICOLON:           return ';';
        case SIM_KEY_SINGLE_QUOTE:        return '\'';
        case SIM_KEY_BACKSLASH:           return '\\';
        case SIM_KEY_LEFT_BACKSLASH:      return '\\';
        case SIM_KEY_COMMA:               return ',';
        case SIM_KEY_PERIOD:              return '.';
        case SIM_KEY_SLASH:               return '/';
        case SIM_KEY_BACKSPACE:           return '\b';
        case SIM_KEY_TAB:                 return '\t';
        case SIM_KEY_ENTER:               return '\r';
        case SIM_KEY_SPACE:               return ' ';
        }
    return k;
}

int
ws_poll(int *valp, int maxus)
{
    SIM_MOUSE_EVENT mev;
    SIM_KEY_EVENT kev;

    //if (maxus > 1000)
        //sim_os_ms_sleep (maxus/1000);

    if (SCPE_OK == vid_poll_mouse (&mev)) {
        unsigned char old_lp_sw = display_lp_sw;
        
        if ((display_lp_sw = mev.b1_state)) {
            ws_lp_x = mev.x_pos;
            ws_lp_y = (ypixels - 1) - mev.y_pos; /* range 0 - (ypixels-1) */
            /* convert to display coordinates */
            ws_lp_x /= pix_size;
            ws_lp_y /= pix_size;
            if (!old_lp_sw && !display_tablet)
                if (vid_set_cursor (1, cross_cursor->width, cross_cursor->height, cross_cursor->data, cross_cursor->mask, cross_cursor->hot_x, cross_cursor->hot_y) == SCPE_OK)
                   old_lp_sw = display_lp_sw = 0;        /* Stop LP event on window focus */
            }
        else {
            ws_lp_x = ws_lp_y = -1;
            if (old_lp_sw && !display_tablet)
                vid_set_cursor (1, arrow_cursor->width, arrow_cursor->height, arrow_cursor->data, arrow_cursor->mask, arrow_cursor->hot_x, arrow_cursor->hot_y);
            }
        vid_set_cursor_position (mev.x_pos, mev.y_pos);
        }
    if (SCPE_OK == vid_poll_kb (&kev)) {
        switch (kev.state) {
            case SIM_KEYPRESS_DOWN:
            case SIM_KEYPRESS_REPEAT:
                display_keydown(kev.key);
                break;
            case SIM_KEYPRESS_UP:
                display_keyup(kev.key);
                break;
            }
        }
    return 1;
}

/* XPM */
static const char *arrow[] = {
  /* width height num_colors chars_per_pixel */
  "    16    16        3            1          7     7",
  /* colors */
  "X c #000000",    /* black */
  ". c #ffffff",    /* white */
  "  c None",
  /* pixels */
  "X               ",
  "XX              ",
  "X.X             ",
  "X..X            ",
  "X...X           ",
  "X....X          ",
  "X.....X         ",
  "X......X        ",
  "X.......X       ",
  "X........X      ",
  "X.....XXXXX     ",
  "X..X..X         ",
  "X.X X..X        ",
  "XX   X..X       ",
  "X     X..X      ",
  "       XX       ",
};

/* XPM */
static const char *cross[] = {
  /* width height num_colors chars_per_pixel hot_x hot_y*/
  "    16    16        3            1          7     7",
  /* colors */
  "X c #000000",    /* black */
  ". c #ffffff",    /* white */
  "  c None",
  /* pixels */
  "      XXXX      ",
  "      X..X      ",
  "      X..X      ",
  "      X..X      ",
  "      X..X      ",
  "      X..X      ",
  "XXXXXXX..XXXXXXX",
  "X..............X",
  "X..............X",
  "XXXXXXX..XXXXXXX",
  "      X..X      ",
  "      X..X      ",
  "      X..X      ",
  "      X..X      ",
  "      X..X      ",
  "      XXXX      ",
  "7,7"
};

static CURSOR *ws_create_cursor(const char *image[])
{
int byte, bit, row, col;
Uint8 *data = NULL;
Uint8 *mask = NULL;
char black, white, transparent;
CURSOR *result = NULL;
int width, height, colors, cpp;
int hot_x = 0, hot_y = 0;

if (4 > sscanf(image[0], "%d %d %d %d %d %d", 
               &width, &height, &colors, &cpp, &hot_x, &hot_y))
    return result;
if ((cpp != 1) || (0 != width%8) || (colors != 3))
    return result;
black = image[1][0];
white = image[2][0];
transparent = image[3][0];
data = (Uint8 *)calloc (1, (width / 8) * height);
mask = (Uint8 *)calloc (1, (width / 8) * height);
if (!data || !mask) {
    free (data);
    free (mask);
    return result;
    }
bit = 7;
byte = 0;
for (row=0; row<height; ++row) {
    for (col=0; col<width; ++col) {
        if (image[colors+1+row][col] == black) {
            data[byte] |= (1 << bit);
            mask[byte] |= (1 << bit);
            }
        else
            if (image[colors+1+row][col] == white) {
                mask[byte] |= (1 << bit);
                }
            else
                if (image[colors+1+row][col] != transparent) {
                    free (data);
                    free (mask);
                    return result;
                    }
        --bit;
        if (bit < 0) {
            ++byte;
            bit = 7;
            }
        }
    }
result = (CURSOR *)calloc (1, sizeof(*result));
if (result) {
    result->data = data;
    result->mask = mask;
    result->width = width;
    result->height = height;
    result->hot_x = hot_x;
    result->hot_y = hot_y;
    }
else {
    free (data);
    free (mask);
    }
return result;
}

static void ws_free_cursor (CURSOR *cursor)
{
if (!cursor)
    return;
free (cursor->data);
free (cursor->mask);
free (cursor);
}

/* called from display layer on first display op */
int
ws_init(const char *name, int xp, int yp, int colors, void *dptr)
{
    int ret = 1;
    
    arrow_cursor = ws_create_cursor (arrow);
    cross_cursor = ws_create_cursor (cross);
    xpixels = xp;
    ypixels = yp;
    window_name = name;
	if (dptr)
		ret = (0 == vid_open ((DEVICE *)dptr, name, xp, yp, SIM_OWN_CURSOR));
    if (ret)
        vid_set_cursor (1, cross_cursor->width, cross_cursor->height, cross_cursor->data, cross_cursor->mask, cross_cursor->hot_x, cross_cursor->hot_y);

    return ret;
}

void
ws_shutdown(void)
{
	if (!arrow_cursor)
		return;
ws_free_cursor(arrow_cursor);
ws_free_cursor(cross_cursor);
arrow_cursor=cross_cursor=0;
vid_close();
}

void
ws_beep(void) {
	vid_beep ();
}

unsigned long
os_elapsed(void)
{
static int tnew;
unsigned long ret;
static uint32 t[2];

t[tnew] = sim_os_msec();
if (t[!tnew] == 0)
    ret = ~0L;                      /* +INF */
else
    ret = (t[tnew] - t[!tnew]) * 1000;/* usecs */
tnew = !tnew;                       /* Ecclesiastes III */
return ret;
}

/*
 * handle keyboard events
 *
 * data switches; 18 -- enough for PDP-1/4/7/9/15 (for munching squares!)
 * 123 456 789 qwe rty uio
 * bit toggled on key up
 * all cleared on space
 *
 * spacewar switches; bit high as long as key down
 * asdf kl;'
 * just where PDP-1 spacewar expects them!
 * key mappings same as MIT Media Lab Java PDP-1 simulator
 * Update for PDP8, move hi bit to correct locations.
 */
unsigned long spacewar_switches = 0;
#ifndef PDP8
#define HI1 040000
#define HI2 0100000
#define HI3 0200000
#define HI4 0400000
#else
#define HI1 0400
#define HI2 01000
#define HI3 02000
#define HI4 04000
#endif

/* here from window system */
void display_keydown(int k)
{
    switch (k) {
    case 'f': case 'F': spacewar_switches |= 01; break; /* torpedos */
    case 'd': case 'D': spacewar_switches |= 02; break; /* engines */
    case 'a': case 'A': spacewar_switches |= 04; break; /* rotate R */
    case 's': case 'S': spacewar_switches |= 010; break; /* rotate L */
    case '\'': case '"': spacewar_switches |= HI1; break; /* torpedos */
    case ';': case ':': spacewar_switches |= HI2; break; /* engines */
    case 'k': case 'K': spacewar_switches |= HI3; break; /* rotate R */
    case 'l': case 'L': spacewar_switches |= HI4; break; /* rotate L */
    default: return;
    }
}

/* here from window system */
void display_keyup(int k)
{
    unsigned long test_switches = cpu_get_switches();


    /* fetch console switches from simulator? */
    switch (k) {
    case 'f': case 'F': spacewar_switches &= ~01; return;
    case 'd': case 'D': spacewar_switches &= ~02; return;
    case 'a': case 'A': spacewar_switches &= ~04; return;
    case 's': case 'S': spacewar_switches &= ~010; return;

    case '\'': case '"': spacewar_switches &= ~HI1; return;
    case ';': case ':': spacewar_switches &= ~HI2; return;
    case 'k': case 'K': spacewar_switches &= ~HI3; return;
    case 'l': case 'L': spacewar_switches &= ~HI4; return;

    case '1': test_switches ^= 1<<17; break;
    case '2': test_switches ^= 1<<16; break;
    case '3': test_switches ^= 1<<15; break;

    case '4': test_switches ^= 1<<14; break;
    case '5': test_switches ^= 1<<13; break;
    case '6': test_switches ^= 1<<12; break;

    case '7': test_switches ^= 1<<11; break;
    case '8': test_switches ^= 1<<10; break;
    case '9': test_switches ^= 1<<9; break;

    case 'q': case 'Q': test_switches ^= 1<<8; break;
    case 'w': case 'W': test_switches ^= 1<<7; break;
    case 'e': case 'E': test_switches ^= 1<<6; break;

    case 'r': case 'R': test_switches ^= 1<<5; break;
    case 't': case 'T': test_switches ^= 1<<4; break;
    case 'y': case 'Y': test_switches ^= 1<<3; break;

    case 'u': case 'U': test_switches ^= 1<<2; break;
    case 'i': case 'I': test_switches ^= 1<<1; break;
    case 'o': case 'O': test_switches ^= 1; break;

    case ' ': test_switches = 0; break;
    default: return;
    }
    cpu_set_switches(test_switches);
}
