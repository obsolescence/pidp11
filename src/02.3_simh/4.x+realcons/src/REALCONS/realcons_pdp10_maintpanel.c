/*  realcons_cpu_pdp10_maintpanel.c: Logic for the upper "maintenance panel".

   Copyright (c) 2014-2016, Joerg Hoppe
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


   05-Sep-2014  JH      created

 */

#define REALCONS_PDP10_C_	// enable private global defintions in realcons_cpu_pdp10.h
#include	"realcons.h"
#include	"realcons_pdp10.h"
#include	"realcons_pdp10_control.h"

// process panel state.
// operates on Blinkenlight_API panel structs,
// but RPC communication is done external
// all input controls are read from panel before call
// all output controls are written back to panel after call
// controls already serviced
t_stat realcons_console_pdp10_maintpanel_service(realcons_console_logic_pdp10_t *_this)
{
	blinkenlight_panel_t *p = _this->realcons->console_model; // alias



	/*** POWER ***/
//	if (realcons_pdp10_control_get(&_this->button_POWER) == 0) {
	if (_this->button_POWER.buttons[0].value == 0 && _this->button_POWER.buttons[0].value_previous == 1) {
		// Power switch transition to POWER OFF: terminate SimH
		// This is drastic, but will teach users not to twiddle with the power switch.
		// when panel is disconnected, panel mode goes to POWERLESS and power switch goes OFF.
		// But shutdown sequence is not initiated, because we're disconnected then.
		SIGNAL_SET(cpusignal_console_halt, 1); // stop execution
		sprintf(_this->realcons->simh_cmd_buffer, "quit"); // do not confirm the quit
		return SCPE_OK;
	}



	/*** LOCK ***/

	// these two flags are referenced by all buttons as "disable"
	_this->console_lock = !!realcons_pdp10_control_get(&_this->button_CONSOLE_LOCK);
	_this->console_datalock = !!realcons_pdp10_control_get(&_this->button_CONSOLE_DATALOCK);

	// controls, which are not set here, remain enabled by default
	// "enable =1"
	// From doc: " the console data lock disables the data and sense switches;
	//	the console lock disables all other buttons except those that are mechanical"
	_this->button_FM_MANUAL.enabled = !_this->console_lock;
	_this->buttons_FM_BLOCK.enabled = !_this->console_lock;
	_this->buttons_SENSE.enabled = !_this->console_datalock;
	_this->button_MEM_OVERLAP_DIS.enabled = !_this->console_lock;
	_this->button_SINGLE_PULSE.enabled = !_this->console_lock;
	_this->button_MARGIN_ENABLE.enabled = !_this->console_lock;
	_this->buttons_READ_IN_DEVICE.enabled = !_this->console_lock;

	/* MI PROG DIS
	 Turn on the triangular light beside MEMORY DATA (turn off the light beside PROGRAM DATA) and
	 inhibit the program from loading any switches or displaying any information in the memory
	 indicators. The indicators will thus continually display the contents of locations selected from
	 the console.
	 */
	_this->button_MI_PROG_DIS.enabled = !_this->console_lock;
	if (_this->button_MI_PROG_DIS.pendingbuttons) {
		_this->memory_indicator_program = 0;
		_this->button_MI_PROG_DIS.pendingbuttons = 0;
	}

	/* the VOLTAGE trafo, VOLTMETER and MARGIN SELECT
	 *
	 * Chapter 10.2:
	 *
	 * VOLTAGE knob middle = nominal voltage onto VOLTMETER
	 *
	 * VOLTMETER:
	 * scale 1: 4.5V .. 5.5V = 0..100%
	 * scale 2: 5V=25%, 10V=50% 15V =75%
	 *
	 * MARGIN SELECT		VOLTAGE		  VOLTMETER
	 * 						%0..100
	 * 0,7   "-15L, -15R"				scale 2: "10V..20V" => 50..100
	 * 1,6   "+10L, +10R"				scale 2: "5..15"   => 25..75
	 * 2,5   "+5L,+5R" 				    sacle 1: "4.5--5,5 => 0..100
	 * -15L, -15R		"10V..20V" => 50..100
	 *
	 * !!! VOLTAGE can only be queried if HOURMETER is ON !!!
	 */
	switch (_this->knob_MARGIN_SELECT.buttons->value) {
	case 0: // "-15L"
	case 7: // "-15R"
		// map VOLTAGE 0..100 -> VOLTMETER 50..100
		_this->VOLTMETER.lamps->value = (_this->knob_MARGIN_VOLTAGE.buttons->value / 2) + 50;
		break;
	case 1: // "+10L"
	case 6: // "+10R"
		// map VOLTAGE 0..100 -> VOLTMETER 25..75
		_this->VOLTMETER.lamps->value = (_this->knob_MARGIN_VOLTAGE.buttons->value / 2) + 25;
		break;
	case 2: // "+5L"
	case 5: // "+5R"
		// map VOLTAGE 0..100 -> VOLTMETER 0..100
		_this->VOLTMETER.lamps->value = _this->knob_MARGIN_VOLTAGE.buttons->value;
		break;
	case 3: // "OFF"
		// VOLTMETER auf 0
		_this->VOLTMETER.lamps->value = 0;
		break;
	case 4: // "PROC MAN"
		// voltmerter auf mittelstellung
		_this->VOLTMETER.lamps->value = 50;
		break;
	}

	/*** speed control
	 * REPEAT and SINGLE PULSE msutbe active, then the pulse speed is given by the speed controls

	 "If SINGLE PULSE is off and any operating key is pressed, then every time the repeat delay can be
	 retriggered, wait a period of time determined by the setting of the speed control and repeat the
	 given key function. The point at which the processor can restart the repeat delay depends upon
	 the type of key function being repeated as follows.

	 For an initiating function the delay starts when the processor stops with RUN off. This
	 is either when the program gives a HALT instruction (STOP PROG) or following the first
	 instruction if SINGLE INST is on.

	 For an independent function the delay starts every time the function is done whether
	 RUN is on or off.

	 A terminating function stops the processor and the delay starts every time the function
	 is repeated. Reset is generally used only to provide a chain of reset pulses
	 on the 10 bus, and stop is used to troubleshoot the clock.

	 In any case continue to repeat the function until REPEAT is turned off. (The function is often
	 repeated once more, but this is noticeable only with very long repeat delays.)

	 The speed control includes a six-position switch that selects the delay range and a
	 potentiometer for fine adjustment within the range. Delay ranges are as follows.

	 Position	Range

	 1	    200 ns to 2 us
	 2	    2 us to 20 us
	 3	    20 us to 500 us
	 4      500 us to 6 ms
	 5	    6 ms to 160 ms
	 6      160 ms to 4 seconds
	 "


	 The simh "throttle" logic works by executing a 1ms delay in intervals
	 of "sim_throt_wait" millisecs.  sim_throt_wait is calculated for "throttle" value and
	 measured host cpu speed. The logic works only for high throttle-values,
	 approx > 100K on modern CPUs.

	 */

	/*** LAMP TEST ***/
	if (_this->realcons->lamp_test) {
		/* lamp test active: terminate? */
		if (!realcons_pdp10_control_get(&_this->button_LAMP_TEST)
				&& !_this->realcons->timer_running_msec[TIMER_TEST])
			realcons_lamp_test(_this->realcons, 0); // end lamptest
	} else {
		/* lamp test inactive: start? */
		if (realcons_pdp10_control_get(&_this->button_LAMP_TEST)
				|| _this->realcons->timer_running_msec[TIMER_TEST])
			realcons_lamp_test(_this->realcons, 1); // begin lamptest
	}
	if (_this->realcons->lamp_test)
		return SCPE_OK; // no more light logic needed

	return 0; // OK

}
