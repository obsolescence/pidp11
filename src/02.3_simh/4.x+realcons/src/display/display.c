/*
 * $Id: display.c,v 1.56 2004/02/03 21:44:34 phil Exp - revised by DAG $
 * Simulator and host O/S independent XY display simulator
 * Phil Budne <phil@ultimate.com>
 * September 2003
 *
 * with changes by Douglas A. Gwyn, 05 Feb. 2004
 *
 * started from PDP-8/E simulator vc8e.c;
 *  This PDP8 Emulator was written by Douglas W. Jones at the
 *  University of Iowa.  It is distributed as freeware, of
 *  uncertain function and uncertain utility.
 */

/*
 * Copyright (c) 2003-2004, Philip L. Budne
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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <limits.h>         /* for USHRT_MAX */
#include "display.h"
#include "ws.h"
#include "sim_defs.h"
/*
 * The user may select (at compile time) how big a window is used to
 * emulate the display.  Using smaller windows saves memory and screen space.
 *
 * Type 30 has 1024x1024 addressing, but only 512x512 visible points.
 * VR14 has only 1024x768 visible points; VR17 has 1024x1024 visible points.
 * VT11 supports 4096x4096 addressing, clipping to the lowest 1024x1024 region.
 * VR48 has 1024x1024 visible points in the main display area and 128x1024
 * visible points in a menu area on the right-hand side (1152x1024 total).
 * VT48 supports 8192x8192 (signed) main-area addressing, clipping to a
 * 1024x1024 window which can be located anywhere within that region.
 * (XXX -- That is what the VT11/VT48 manuals say; however, evidence suggests
 * that the VT11 may actually support 8192x8192 (signed) addressing too.)
 */

/* Define the default display type (if display_init() not called) */
#ifndef DISPLAY_TYPE
#define DISPLAY_TYPE DIS_TYPE30
#endif /* DISPLAY_TYPE not defined */

/* Define the default window type (if display_init() not called) */
#ifndef WINDOW_TYPE
#define WINDOW_TYPE WIN_NORM
#endif /* WINDOW_TYPE not defined */


/* select a default resolution if display_init() not called */
/* XXX keep in struct display? */
#ifndef PIX_SCALE
#define PIX_SCALE RES_FULL
#endif /* PIX_SCALE not defined */

/* select a default light-pen hit radius if display_init() not called */
#ifndef PEN_RADIUS
#define PEN_RADIUS 4
#endif /* PEN_RADIUS not defined */


#ifndef PIX_SIZE
#define PIX_SIZE 1
#endif

/*
 * note: displays can have up to two different colors (eg VR20)
 * each color can be made up of any number of phosphors
 * with different colors and decay characteristics (eg Type 30)
 * Change ISS (2018). It is now possible to create complex phosphor
 * characteristics by using a 3 decay constants for RGB.
 * This only requires 4 entries.
 * 3 RGB decay constants
 * 1 initial color value.
 */

#define ELEMENTS(X) (sizeof(X)/sizeof(X[0]))

struct phosphor {
    double colr[3];
    int32 level;           /* Inital RGB pixel value */
};

struct display {
    enum display_type type;
    const char *name;
    struct phosphor *phos;
    short xpoints, ypoints;
};



/*	Color tables. Expected display surface format is ARGB. Some systems do not support A. Decay parameter order is is BGR  */
/*	This is a little endian format ... LSByte first. Howvever, very few system are now big endian or will support both. */


/*
	* original phosphor constants from Raphael Nabet's XMame 0.72.1 PDP-1 sim.
	* not even sure Type30 really used P17 (guess by Daniel P. B. Smith)
	* This code displays short persistance 'electric blue' with a green decay.
	* Judged from color photos on the web.
 */
static struct phosphor p17[] = {{0.8,0.998,0.8,0x8080ff}};

	/* green phosphor for VR14, VR17, VR20 */
static struct phosphor p29[] = {{0.0,0.9,0.0,0xff00}};

    /* P40 blue-white spot with yellow-green decay (.045s to 10%?) */
static struct phosphor p40[] = {{0.8,0.98,0.98,0xb0b0ff}};

	/* "red+green" -- until real VR20 phosphor type/number/constants known */
	/* Only used with the VC8(e) display hardware. CO bit changes (masks) pxval in vid_setpixel */
	
static struct phosphor pred[] = { {0.0,0.9,0.9,0xffff00} };

static struct display displays[] = {
   /*
     * TX-0
     * 
     *
     * Unknown manufacturer
     * 
     * 12" tube, 
     * maximum dot size ???
     * 50us point plot time (20,000 points/sec)
     * P17 Phosphor??? Two phosphor layers:
     * fast blue (.05s half life), and slow green (.2s half life)
     * 
     * 
     */
    { DIS_TX0, "MIT TX-0", p17, 512, 512 },

    
    /*
     * Type 30
     * PDP-1/4/5/8/9/10 "Precision CRT" display system
     *
     * Raytheon 16ADP7A CRT?
     * web searches for 16ADP7 finds useful information!!
     * 16" tube, 14 3/8" square raster
     * maximum dot size .015"
     * 50us point plot time (20,000 points/sec)
     * P17 Phosphor??? Two phosphor layers:
     * fast blue (.05s half life), and slow green (.2s half life)
     * 360 lb
     * 7A at 115+-10V 60Hz
     */
    { DIS_TYPE30, "Type 30", p17, 1024, 1024 },

    /*
     * VR14
     * used w/ GT40/44, AX08, VC8E
     *
     * Viewable area 6.75" x 9"
     * 12" diagonal
     * brightness >= 30 fL
     * dot size .02" (20 mils)
     * settle time:
     *  full screen 18us to +/-1 spot diameter
     *  .1" change 1us to +/-.5 spot diameter
     * weight 75lb
     */
    { DIS_VR14, "VR14", p29,  1024, 768 },

    /*
     * VR17
     * used w/ GT40/44, AX08, VC8E
     *
     * Viewable area 9.25" x 9.25"
     * 17" diagonal
     * dot size .02" (20 mils)
     * brightness >= 25 fL
     * phosphor: P39 doped for IR light pen use
     * light pen: Type 375
     * weight 85lb
     */
    { DIS_VR17, "VR17", p29, 1024, 1024 },

    /*
     * VR20
     * on VC8E
     * Two colors!!
     */
    { DIS_VR20, "VR20", pred, 1024, 1024 },

    /*
     * VR48
     * (on VT48 in VS60)
     * from Douglas A. Gwyn 23 Nov. 2003
     *
     * Viewable area 12" x 12", plus 1.5" x 12" menu area on right-hand side
     * 21" diagonal
     * dot size <= .01" (10 mils)
     * brightness >= 31 fL
     * phosphor: P40 (blue-white fluorescence with yellow-green phosphorescence)
     * light pen: Type 377A (with tip switch)
     * driving circuitry separate
     * (normally under table on which CRT is mounted)
     */
    { DIS_VR48, "VR48", p40, 1024+VR48_GUTTER+128, 1024 },

    /*
     * Type 340 Display system
     * on PDP-4/6/7/9/10
     *
     * 1024x1024
     * 9 3/8" raster (.01" dot pitch)
     * 0,0 at lower left
     * 8 intensity levels
     */
    { DIS_TYPE340, "Type 340", p17, 1024, 1024 }
};


/*
 * Display Device Implementation
 */

/*
	* 
 */

static int initialized = 0;


/*
 * can be changed with display_lp_radius()
 */
static long scaled_pen_radius_squared;

/* run-time -- set by display_init() */
static int xpoints, ypoints;
static int xpixels, ypixels;
static int refresh_rate;
static int refresh_interval;
static int ncolors;
static enum display_type display_type;
static enum window_type window_type;
static int scale;
#ifdef  __cplusplus
extern "C" {
#endif

extern double *colmap;
extern int32 pxval;
extern t_stat vid_setpixel(int ix,int iy,int level,int color);

#ifdef  __cplusplus
}
#endif


void
display_lp_radius(int r)
{
    r /= scale;
    scaled_pen_radius_squared = r * r;
}

/*
 * here to to dynamically adjust interval for examination
 * of elapsed vs. simulated time, and fritter away
 * any extra wall-clock time without eating CPU
 */

/*
 * more parameters!
 */

/*
 * upper bound for elapsed time between elapsed time checks.
 * if more than MAXELAPSED microseconds elapsed while simulating
 * delay_check simulated microseconds, decrease delay_check.
 */
#define MAXELAPSED 100000       /* 10Hz */

/*
 * lower bound for elapsed time between elapsed time checks.
 * if fewer than MINELAPSED microseconds elapsed while simulating
 * delay_check simulated microseconds, increase delay_check.
 */
#define MINELAPSED 50000        /* 20Hz */

/*
 * upper bound for delay (sleep/poll).
 * If difference between elapsed time and simulated time is
 * larger than MAXDELAY microseconds, decrease delay_check.
 *
 * since delay is elapsed time - simulated time, MAXDELAY
 * should be <= MAXELAPSED
 */
#ifndef MAXDELAY
#define MAXDELAY 100000         /* 100ms */
#endif /* MAXDELAY not defined */

/*
 * lower bound for delay (sleep/poll).
 * If difference between elapsed time and simulated time is
 * smaller than MINDELAY microseconds, increase delay_check.
 *
 * since delay is elapsed time - simulated time, MINDELAY
 * should be <= MINELAPSED
 */
#ifndef MINDELAY
#define MINDELAY 50000          /* 50ms */
#endif /* MINDELAY not defined */

/*
 * Initial amount of simulated time to elapse before polling.
 * Value is very low to ensure polling occurs on slow systems.
 * Fast systems should ramp up quickly.
 */
#ifndef INITIAL_DELAY_CHECK
#define INITIAL_DELAY_CHECK 1000    /* 1ms */
#endif /* INITIAL_DELAY_CHECK */

/*
 * gain factor (2**-GAINSHIFT) for adjustment of adjustment
 * of delay_check
 */
#ifndef GAINSHIFT
#define GAINSHIFT 3         /* gain=0.125 (12.5%) */
#endif /* GAINSHIFT not defined */

static void
display_delay(int t, int slowdown)
{
    /* how often (in simulated us) to poll/check for delay */
    static unsigned long delay_check = INITIAL_DELAY_CHECK;

    /* accumulated simulated time */
    static unsigned long sim_time = 0;
    unsigned long elapsed;
    long delay;

    sim_time += t;
    if (sim_time < delay_check)
        return;

    elapsed = os_elapsed();     /* read and reset elapsed timer */
    if (elapsed == ~0L) {       /* first time thru? */
        slowdown = 0;           /* no adjustments */
        elapsed = sim_time;
        }

    /*
     * get delta between elapsed (real) time, and simulated time.
     * if simulated time running faster, we need to slow things down (delay)
     */
    if (slowdown)
        delay = sim_time - elapsed;
    else
        delay = 0;              /* just poll */

#ifdef DEBUG_DELAY2
    printf("sim %d elapsed %d delay %d\r\n", sim_time, elapsed, delay);
#endif

    /*
     * Try to keep the elapsed (real world) time between checks for
     * delay (and poll for window system events) bounded between
     * MAXELAPSED and MINELAPSED.  Also tries to keep
     * delay/poll time bounded between MAXDELAY and MINDELAY -- large
     * delays make the simulation spastic, while very small ones are
     * inefficient (too many system calls) and tend to be inaccurate
     * (operating systems have a certain granularity for time
     * measurement, and when you try to sleep/poll for very short
     * amounts of time, the noise will dominate).
     *
     * delay_check period may be adjusted often, and oscillate.  There
     * is no single "right value", the important things are to keep
     * the delay time and max poll intervals bounded, and responsive
     * to system load.
     */
    if (elapsed > MAXELAPSED || delay > MAXDELAY) {
        /* too much elapsed time passed, or delay too large; shrink interval */
        if (delay_check > 1) {
            delay_check -= delay_check>>GAINSHIFT;
#ifdef DEBUG_DELAY
            printf("reduced period to %d\r\n", delay_check);
#endif /* DEBUG_DELAY defined */
            }
        }
    else 
        if ((elapsed < MINELAPSED) || (slowdown && (delay < MINDELAY))) {
            /* too little elapsed time passed, or delta very small */
            int gain = delay_check>>GAINSHIFT;

            if (gain == 0)
                gain = 1;           /* make sure some change made! */
            delay_check += gain;
#ifdef DEBUG_DELAY
            printf("increased period to %d\r\n", delay_check);
#endif /* DEBUG_DELAY defined */
            }
    if (delay < 0)
        delay = 0;
    /* else if delay < MINDELAY, clamp at MINDELAY??? */

    sim_time = 0;                   /* reset simulated time clock */

    /*
     * delay (poll/sleep) time included in next "elapsed" period
     * (clock not reset after a delay)
     */
} /* display_delay */

/*
	* This function has been left in as its is called by a number of simulators
	* The display decay process is now managed via a separate thread. See sim_video.c.
	* This function still provides a delay and will ensure that the
	* display system is initialised if this is the first called function.
 */

int
display_age(int t,          /* simulated us since last call */
        int slowdown)       /* slowdown to simulated speed */
{
    static int elapsed = 0;
    static int refresh_elapsed = 0; /* in units of DELAY_UNIT bounded by refresh_interval */
    int changed;

	if (!initialized && !display_init(DISPLAY_TYPE, PIX_SCALE, NULL))
        return 0;

	if (slowdown)
		display_delay(t, slowdown);

    changed = 0;
	display_sync();
	return 0;

} /* display_age */


/* (0,0) is lower left */
static int
intensify(int x,            /* 0..xpixels */
      int y,                /* 0..ypixels */
      int level,            /* 0..MAXLEVEL */
      int color)            /* for VR20! 0 or 1 */
{
 
int pix_size=PIX_SIZE;

    if (x < 0 || x >= xpixels || y < pix_size-1 || y >= ypixels)
        return 0;           /* limit to display */

    if (pix_size > 1) {
        int i, j;
        
        for (i=0; i<pix_size; i++)
            for (j=0; j<pix_size; j++)
                		vid_setpixel(i+x, j+ypixels-y-1, level, color);
        }
    else
		vid_setpixel(x, ypixels-y-1, level, color);		/* new code */
	
	return 0;
}

int
display_point(int x,        /* 0..xpixels (unscaled) */
          int y,            /* 0..ypixels (unscaled) */
          int level,        /* DISPLAY_INT_xxx */
          int color)        /* for VR20! 0 or 1 */
{
    long lx, ly;

	/* This should never happen!! */
	
    if (!initialized)
        return 0;

    /* scale x and y to the displayed number of pixels */
    /* handle common cases quickly */
    if (scale > 1) {
        if (scale == 2) {
            x >>= 1;
            y >>= 1;
            }
        else {
            x /= scale;
            y /= scale;
            }
        }

#if DISPLAY_INT_MIN > 0
    level -= DISPLAY_INT_MIN;       /* make zero based */
#endif
    intensify(x, y, level, color);
 
    if (ws_lp_x == -1 || ws_lp_y == -1)
        return 0;
    lx = x - ws_lp_x;
    ly = y - ws_lp_y;
    return lx*lx + ly*ly <= scaled_pen_radius_squared;
} /* display_point */



static struct display *
find_type(enum display_type type)
{
    int i;
    struct display *dp;

    for (i = 0, dp = displays; i < ELEMENTS(displays); i++, dp++)
        if (dp->type == type)
            return dp;
    return NULL;
}

int
display_init(enum display_type type, int sf, void *dptr)
{
    static int init_failed = 0;
    struct display *dp;

    if (initialized) {
        /* cannot change type once started */
        /* XXX say something???? */
        return type == display_type;
        }

    if (init_failed)
        return 0;               /* avoid thrashing */

    init_failed = 1;            /* assume the worst */
    dp = find_type(type);
    if (!dp) {
        fprintf(stderr, "Unknown display type %d\r\n", (int)type);
        return 0;
        }

    display_type = type;
    scale = sf;
	colmap = dp->phos->colr;
	pxval = dp->phos->level;

    xpoints = dp->xpoints;
    ypoints = dp->ypoints;

    /* increase scale factor if won't fit on desktop? */
    xpixels = xpoints / scale;
    ypixels = ypoints / scale;

    /* set default pen radius now that scale is set */
    display_lp_radius(PEN_RADIUS);

    ncolors = 1;
	
	if (!ws_init(dp->name, xpixels, ypixels, ncolors, dptr))
        return 0;

	init_failed = 0;
    return initialized = 1;
}
void
display_reset(void)
{
    /* XXX tear down window? just clear it? */
	initialized=0;
}

void
display_sync(void)
{
    ws_poll (NULL, 0);

}

void
display_beep(void)
{
    ws_beep();
}

int
display_xpoints(void)
{
    return xpoints;
}

int
display_ypoints(void)
{
    return ypoints;
}

int
display_scale(void)
{
    return scale;
}

