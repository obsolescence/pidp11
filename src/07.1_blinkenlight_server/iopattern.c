/* iopattern.c: transform Blinkenlight API values into muxed BlinkenBus analog levels

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



 Problem:
 panel lamps may appear "steady dimmed", as they are switched ON/OFF very fast
 by a running CPU.
 To achive this effect, the SimH simulator has to update IO at high freqency.
 This wastes CPU power and network bandwidth, and does not guard against
 "stroboscipic" resonance effect between looping CPU activty and update rate,
 resulting in visual lamp flicker.

 Solution:
 the Blinkenlight API output commands are held in a ringbuffer, so at any time
 the mean value for an arbitrary time interval can be build.
 Those analog brightness levels are used to drive a multiplexing logic,
 which for each analog value gives an predefined ON/OFF pattern onto the
 BlinkenBus outputs.

 2 threads are implemented:
 - lowpass thread: updates the analog values to dispaly at quite low freqency:
 20 to 50 Hz. For 32 brightnesslevels, 32 SimH messagesm ust be sampled.
 So if REALCONS is running at 1ms intervals, low pass interval must be at least 32ms.

 - muxthread
 This drives the blinkenbus multiplexing in high speed.
 A "cache" contains the should-be state of all BlinkenBus registers and is
 transfered to the driver in optimized way.
 For 32 brightness levels there are 32 mux phases,
 each contains one on/off state of all controls.

 */
#define IOPATTERN_C_
#ifndef WIN32

#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <assert.h>

#include "print.h"
#include "blinkenbus.h"
#include "iopattern.h"
#include "main.h"   // global blinkenlight_panel_list

// one cache for outputs per display phase
blinkenbus_map_t blinkenbus_output_caches[IOPATTERN_OUTPUT_PHASES];

// only one cache for inputs
blinkenbus_map_t blinkenbus_input_cache;

/*
 * low frequency thread sets brightnesses
 */
pthread_t iopattern_update_thread;
int iopattern_update_thread_terminate = 0;

// high speed multiplexing to get brightness levels
pthread_t blinkenbus_mux_thread;
int blinkenbus_mux_thread_terminate = 0;

// biometrics
unsigned blinkenbus_min_cycle_time_ns, blinkenbus_max_cycle_time_ns;

#if 0
/*
 * Table for bitvalues for different display phases and a given brigthness level.
 * Generated with "LedPatterns.exe"
 * The relation between perceived brightness and required pattern is normally non-linear.
 * This table depends on the driver electronic and needs to be fine-tuned.
 * Brightness-to-duty-cycle function: "Linear"
 * Pattern style: "Distributed".
 */
char brightness_phase_lookup[16][15] =
{ //
    {   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, //  0/15 =  0%
    {   1,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, //  1/15 =  7%
    {   1,0,0,0,0,0,0,0,1,0,0,0,0,0,0}, //  2/15 = 13%
    {   1,0,0,0,0,1,0,0,0,0,1,0,0,0,0}, //  3/15 = 20%
    {   1,0,0,0,1,0,0,0,1,0,0,1,0,0,0}, //  4/15 = 27%
    {   1,0,0,1,0,0,1,0,0,1,0,0,1,0,0}, //  5/15 = 33%
    {   1,0,1,0,0,1,0,0,1,0,1,0,1,0,0}, //  6/15 = 40%
    {   1,0,1,0,1,0,1,0,0,1,0,1,0,1,0}, //  7/15 = 47%
    {   1,0,1,0,1,0,1,0,1,1,0,1,0,1,0}, //  8/15 = 53%
    {   1,0,1,1,0,1,0,1,1,0,1,0,1,1,0}, //  9/15 = 60%
    {   1,0,1,1,1,0,1,0,1,1,1,0,1,0,1}, // 10/15 = 67%
    {   1,1,0,1,1,1,0,1,1,0,1,1,1,0,1}, // 11/15 = 73%
    {   1,1,1,0,1,1,1,0,1,1,1,1,1,0,1}, // 12/15 = 80%
    {   1,1,1,1,0,1,1,1,1,1,1,0,1,1,1}, // 13/15 = 87%
    {   1,1,1,1,1,1,1,0,1,1,1,1,1,1,1}, // 14/15 = 93%
    {   1,1,1,1,1,1,1,1,1,1,1,1,1,1,1} // 15/15 = 100%
};

#endif

#if 1
/*
 * Table for bitvalues for different display phases and a given brigthness level.
 * Generated with "LedPatterns.exe"
 * The relation between perceived brightness and required pattern is normally non-linear.
 * This table depends on the driver electronic and needs to be fine-tuned.
 * Brightness-to-duty-cycle function: "Linear"
 * Pattern style: "2xPWM".
 */
#define GPIOPATTERN_LED_BRIGHTNESS_LEVELS 32 // brightness levels
#define GPIOPATTERN_LED_BRIGHTNESS_PHASES 31 // normally, <n> brightness levels need <n>-1 phases
char brightness_phase_lookup[32][31] =
{ //
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, //  0/31 =  0%
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, //  1/31 =  3%
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, //  2/31 =  6%
  { 1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0 }, //  3/31 = 10%
  { 1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0 }, //  4/31 = 13%
  { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0 }, //  5/31 = 16%
  { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0 }, //  6/31 = 19%
  { 1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0 }, //  7/31 = 23%
  { 1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0 }, //  8/31 = 26%
  { 1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0 }, //  9/31 = 29%
  { 1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0 }, // 10/31 = 32%
  { 1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0 }, // 11/31 = 35%
  { 1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0 }, // 12/31 = 39%
  { 1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0 }, // 13/31 = 42%
  { 1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0 }, // 14/31 = 45%
  { 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0 }, // 15/31 = 48%
  { 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0 }, // 16/31 = 52%
  { 1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0 }, // 17/31 = 55%
  { 1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0 }, // 18/31 = 58%
  { 1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,0 }, // 19/31 = 61%
  { 1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0 }, // 20/31 = 65%
  { 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0 }, // 21/31 = 68%
  { 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0 }, // 22/31 = 71%
  { 1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0 }, // 23/31 = 74%
  { 1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0 }, // 24/31 = 77%
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0 }, // 25/31 = 81%
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0 }, // 26/31 = 84%
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0 }, // 27/31 = 87%
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0 }, // 28/31 = 90%
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0 }, // 29/31 = 94%
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }, // 30/31 = 97%
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }  // 31/31 = 100%
} ;

#endif

#if 0
/*
 * Table for bitvalues for different display phases and a given brigthness level.
 * Generated with "LedPatterns.exe"
 * The relation between perceived brightness and required pattern is normally non-linear.
 * This table depends on the driver electronic and needs to be fine-tuned.
 * Brightness-to-duty-cycle function: "Logarithmic"
 *      logarithmic shift: -3.
 * Pattern style: "2xPWM".
 */
//#define GPIOPATTERN_LED_BRIGHTNESS_LEVELS 32 // brightness levels
//#define GPIOPATTERN_LED_BRIGHTNESS_PHASES 32 // normally, <n> brightness levels need <n>-1 phases
char brightness_phase_lookup[32][32] =
{ //  least duty cycle = 2, so always double frequency is guaranteed
  { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, //  0/32 =  0%
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, //  2/32 =  6%
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, //  2/32 =  6%
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, //  2/32 =  6%
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, //  2/32 =  6%
  { 1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, //  2/32 =  6%
  { 1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0 }, //  3/32 =  9%
  { 1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0 }, //  3/32 =  9%
  { 1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0 }, //  3/32 =  9%
  { 1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0 }, //  3/32 =  9%
  { 1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, //  4/32 = 12%
  { 1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, //  4/32 = 12%
  { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0 }, //  5/32 = 16%
  { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,0,0,0,0,0 }, //  5/32 = 16%
  { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0 }, //  6/32 = 19%
  { 1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0 }, //  6/32 = 19%
  { 1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,0,0,0,0,0 }, //  7/32 = 22%
  { 1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0 }, //  8/32 = 25%
  { 1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0 }, //  9/32 = 28%
  { 1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,0,0,0,0,0 }, //  9/32 = 28%
  { 1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0 }, // 10/32 = 31%
  { 1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0 }, // 12/32 = 38%
  { 1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0 }, // 13/32 = 41%
  { 1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,0 }, // 14/32 = 44%
  { 1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0 }, // 16/32 = 50%
  { 1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0 }, // 17/32 = 53%
  { 1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0 }, // 19/32 = 59%
  { 1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0 }, // 21/32 = 66%
  { 1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0 }, // 24/32 = 75%
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0 }, // 26/32 = 81%
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0 }, // 29/32 = 91%
  { 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1 }  // 32/32 = 100%
} ;

#endif

/*
 * A PWM pattern is used, because it darstically reduces switche events write cycles
 * on the BlinkenBus.
 * MUX Frequency must be 2x perception freqeuncy = 60Hz
 */

/*
 * - averages the Blinkenlight API outputs,
 * - generates the LED brightness patterns
 * - swaps the double buffer
 *
 * This here should run in a low-frequency thread.
 * recommended frequency: 50 Hz, much lower than SimH update rate
 *
 */

// override value with selftest/"power off" values
static uint64_t panel_mode_control_value(blinkenlight_control_t *c, uint64_t value)
{
    unsigned panel_mode = c->panel->mode;

    if (panel_mode == RPC_PARAM_VALUE_PANEL_MODE_ALLTEST
            || (panel_mode == RPC_PARAM_VALUE_PANEL_MODE_LAMPTEST && c->type == output_lamp))
        return 0xffffffffffffffff; // selftest:
    if (panel_mode == RPC_PARAM_VALUE_PANEL_MODE_POWERLESS && c->type == output_lamp)
        return 0; // all OFF

    return value; // default: unchanged
}

void *iopattern_update_outputs(void *arg)
{
    int *terminate = (int *) arg;
    uint64_t now_us;
    unsigned i_panel;
    unsigned i_control;

    do { // at least one cycle, for non-thread call
        struct timespec wait;

        wait.tv_sec = 0;
        wait.tv_nsec = IOPATTERN_UPDATE_PERIOD_US * 1000;
        // wait for one period
        nanosleep(&wait, NULL);

        now_us = historybuffer_now_us();

        // output mount bits for all panels, all controls and all phases
        // into phase cache page
        for (i_panel = 0; i_panel < blinkenlight_panel_list->panels_count; i_panel++) {
            blinkenlight_panel_t *p = &(blinkenlight_panel_list->panels[i_panel]);

            for (i_control = 0; i_control < p->controls_count; i_control++) {
                blinkenlight_control_t *c = &p->controls[i_control];
                unsigned bitidx;
                unsigned phase;
                if (c->is_input)
                    continue;

                // fetch  shift
                // get averaged values
                if (c->fmax == 0 || c->encoding != binary) {
                    // no averaging: brightness only 0 or 1, all phases identical
                    // Must be used for encoding BITPOSITION (PDP-11/70 USER/SUPER/KERNEL)
                    for (phase = 0; phase < IOPATTERN_OUTPUT_PHASES; phase++) {
                        uint64_t value = panel_mode_control_value(c, c->value);
                        blinkenbus_outputcontrol_to_cache(blinkenbus_output_caches[phase], c,
                                value);
//                    blinkenbus_outputcontrol_to_cache(blinkenbus_output_caches[phase], c, c->value >> phase) ;
                    }
                } else {
                    // average each bit
                    historybuffer_get_average_vals(c->history, 1000000 / c->fmax, now_us, /*bitmode*/
                    1);

                    /*
                     // construct un-averaged value
                     for (bitidx = 0; bitidx < c->value_bitlen; bitidx++)
                     c->averaged_value_bits[bitidx] = (bitidx + 1) * 8 * ((c->value >> bitidx) & 1);
                     //MSB s brighter
                     */

                    // build the display value from the low-passed bits.
                    // for all display phases :
                    for (phase = 0; phase < IOPATTERN_OUTPUT_PHASES; phase++) {
                        unsigned value = 0;
                        // for all bits :
                        for (bitidx = 0; bitidx < c->value_bitlen; bitidx++) {
                            // mount phase bit
                            unsigned bit_brightness = ((unsigned) (c->averaged_value_bits[bitidx])
                                    * (IOPATTERN_OUTPUT_BRIGHTNESS_LEVELS)) / 256; // from 0.. 255 to
                            // fixup 0/1 flicker: very low brightness rounded to 0?
                            if (bit_brightness == 0 && c->averaged_value_bits[bitidx] > 0)
                                bit_brightness = 1 ;
//bit_brightness = 1;
                            assert(bit_brightness < IOPATTERN_OUTPUT_BRIGHTNESS_LEVELS);
                            if (brightness_phase_lookup[bit_brightness][phase])
                                value |= 1 << bitidx;
                        }
                        // output to cache phase

                        // override low passed pattern by lamp test
                        value = panel_mode_control_value(c, value);

                        blinkenbus_outputcontrol_to_cache(blinkenbus_output_caches[phase], c,
                                value);
                    }
                }
            }
        }
    } while (*terminate == 0) ;
    return 0;
}

/*
 * Thread for output multiplexing over the blinkenbus.
 * a set of "phase count" output cache pages for the BlinkenBus is written to the
 * Blinkenboards in circular order.
 */

void *iopattern_blinkenbus_mux(void *arg)
{
    int *terminate = arg;
    unsigned phase = 0;

    do { // at least one cycle, for non-thread call
        struct timespec clock_start, clock_end;
        unsigned char *cur_cache;

        // begin of I/O
        clock_gettime(CLOCK_MONOTONIC, &clock_start);

        cur_cache = blinkenbus_output_caches[phase];

        blinkenbus_cache_to_blinkenboards_outputs(cur_cache, /*force_all*/0);

        // inc to next display phase
        phase = (phase + 1) % IOPATTERN_OUTPUT_PHASES;

        if (phase == 0) {
            // sense all inputs, with lower frequency
            blinkenbus_cache_from_blinkenboards_inputs(blinkenbus_input_cache);

        }

        // end of I/O
        clock_gettime(CLOCK_MONOTONIC, &clock_end);

        // calculate remaining time to wait: start + interval - end
        {
            int64_t blinkenbus_cycle_time_ns;
            int64_t wait_ns;

            // elapsed time to write all bus registers
            blinkenbus_cycle_time_ns =
                    ((uint64_t) clock_end.tv_sec * 1000000000 + clock_end.tv_nsec)
                            - ((uint64_t) clock_start.tv_sec * 1000000000 + clock_start.tv_nsec);

            // biometrics
            if (blinkenbus_min_cycle_time_ns == 0
                    || blinkenbus_cycle_time_ns < blinkenbus_min_cycle_time_ns)
                blinkenbus_min_cycle_time_ns = blinkenbus_cycle_time_ns;
            if (blinkenbus_max_cycle_time_ns == 0
                    || blinkenbus_cycle_time_ns > blinkenbus_max_cycle_time_ns)
                blinkenbus_max_cycle_time_ns = blinkenbus_cycle_time_ns;

            // wait for one period
            wait_ns = (long) 1000 * IOPATTERN_OUTPUT_MUX_PERIOD_US - blinkenbus_cycle_time_ns;
            if (wait_ns > 0) {
                struct timespec wait;
                wait.tv_sec = 0;
                wait.tv_nsec = wait_ns;
                nanosleep(&wait, NULL);
            }
        }
    } while (*terminate == 0) ;
}

/*
 * Setup BlinkenBus, start threads
 */
void iopattern_init()
{
    unsigned phase;
    int res;

    blinkenbus_min_cycle_time_ns = blinkenbus_max_cycle_time_ns = 0;

    blinkenbus_init(); // panel config must be known

    // intialize all output cache phases with image of blinkenbus register space
    for (phase = 0; phase < IOPATTERN_OUTPUT_PHASES; phase++)
        blinkenbus_cache_from_blinkenboards_outputs(blinkenbus_output_caches[phase]);

    // sample inputs 1st time, so they are guaranteed to be valid
    blinkenbus_cache_from_blinkenboards_inputs(blinkenbus_input_cache);

    // start threads
    res = pthread_create(&blinkenbus_mux_thread, NULL, &iopattern_blinkenbus_mux,
            (void *) &blinkenbus_mux_thread_terminate);
    if (res) {
        fprintf(stderr, "Error creating blinkenbus_mux thread, return code %d\n", res);
        exit(EXIT_FAILURE);
    }
    print(LOG_NOTICE, "Created \"blinkenbus_mux\" thread\n");

    res = pthread_create(&iopattern_update_thread, NULL, &iopattern_update_outputs,
            (void *) &iopattern_update_thread_terminate);
    if (res) {
        fprintf(stderr, "Error creating blinkenbus_update thread, return code %d\n", res);
        exit(EXIT_FAILURE);
    }
    print(LOG_NOTICE, "Created \"blinkenbus_update\" thread\n");

}


#endif
