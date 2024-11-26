/* iopattern.h: transform Blinkenlight API values into muxed BlinkenBus analog levels

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

 20-Mar-2012  JH      created
 */
#ifndef IOPATTERN_H_
#define IOPATTERN_H_

#include "blinkenbus.h"

#define IOPATTERN_UPDATE_PERIOD_US 20000 // update frequency 50 Hz

//#define IOPATTERN_OUTPUT_MUX_PERIOD_US 1500  // MUX frequency 500HZ: 16 levels a 30 Hz
#define IOPATTERN_OUTPUT_MUX_PERIOD_US 1000  // MUX frequency 1 kHz


#define IOPATTERN_OUTPUT_BRIGHTNESS_LEVELS   32  // brightness levels. Not changeable without code rework
//#define IOPATTERN_OUTPUT_BRIGHTNESS_LEVELS 16
#define IOPATTERN_OUTPUT_PHASES   31
//#define IOPATTERN_OUTPUT_PHASES 15
// 32 levels are made with 31 display phases

#ifndef IOPATTERN_C_
extern blinkenbus_map_t blinkenbus_output_caches[IOPATTERN_OUTPUT_PHASES];
extern blinkenbus_map_t blinkenbus_input_cache;
extern unsigned long blinkenbus_min_cycle_time_ns, blinkenbus_max_cycle_time_ns ;

#endif

void iopattern_init();

//void iopattern_blinkenbus_mux(int *terminate) ;

#endif

