/* gpiopattern.h: pattern generator, transforms data between Blinkenlight APi and gpio-MUX

   Copyright (c) 2016, Joerg Hoppe
   j_hoppe@t-online.de, www.retrocmp.com

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
   JOERG HOPPE BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


   14-Mar-2016  JH      created
*/
#ifndef GPIOPATTERN_H_
#define GPIOPATTERN_H_

#include <stdint.h>
#include <pthread.h>

// protect the readidx/writeidx swap
extern pthread_mutex_t gpiopattern_swap_lock;

#include "blinkenlight_panels.h"

//#define GPIOPATTERN_UPDATE_PERIOD_US 20000  // 1/50 sec for screen update
#define GPIOPATTERN_UPDATE_PERIOD_US 50000  // 1/20 sec for screen update
// in one cycle more than GPIOPATTERN_LED_BRIGHTNESS_LEVELS events from server must be
// sampled, else loss of resolution

#define GPIOPATTERN_LED_BRIGHTNESS_LEVELS	32	// brightness levels. Not changeable without code rework
//#define GPIOPATTERN_LED_BRIGHTNESS_PHASES	64
#define GPIOPATTERN_LED_BRIGHTNESS_PHASES	31
// 32 levels are made with 31 display phases


#ifndef GPIOPATTERN_C_
extern blinkenlight_panel_t *gpiopattern_blinkenlight_panel ;

extern volatile uint32_t gpio_switchstatus[3] ; // bitfields: 3 rows of up to 12 switches
// extern volatile uint32_t gpio_ledstatus[8] ; // bitfields: 8 ledrows of up to 12 LEDs

extern int gpiopattern_ledstatus_phases_readidx ; // read page, used by GPIO mux
extern int gpiopattern_ledstatus_phases_writeidx ; // writepage page, written from Blinkenlight API
extern volatile uint32_t gpiopattern_ledstatus_phases[2][GPIOPATTERN_LED_BRIGHTNESS_PHASES][8] ;

#endif

// void *gpiopattern_update_leds(int *terminate) ;


#endif
