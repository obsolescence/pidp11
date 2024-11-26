/*  realcons_cpu_pdp10_operpanel.c: Logic for the lower "operator panel".

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


   12-Mar-2016  JH      64bit printf/scanf fmt changed to PRI*64 (inttypes.h)
   05-Sep-2014  JH      created
 */

#define REALCONS_PDP10_C_	// enable private global defintions in realcons_cpu_pdp10.h

#include <inttypes.h>
#include "realcons.h"
#include "realcons_pdp10.h"
#include "realcons_pdp10_control.h"

// process panel state.
// operates on Blinkenlight_API panel structs,
// but RPC communication is done external
// all input controls are read from panel before call
// all output controls are written back to panel after call
// controls already serviced

t_stat realcons_pdp10_operpanel_service(realcons_console_logic_pdp10_t *_this)
{
	unsigned single_inst; //

	// ----- Default ENABLE logic --------------
	// controls, which are nort reclaced here, remain enbaled by default
	// "enable =1"
	// "In the upper half of the operator panel are four rows of indicators, and below them are three
	// rows of two-position keys and switches. Physically both are pushbuttons, but the keys are
	// momentary contact whereas the switches are alternate action.""
	_this->button_PAGING_EXEC.enabled = !_this->console_lock;
	_this->button_PAGING_USER.enabled = !_this->console_lock;

	_this->button_SINGLE_PULSER.enabled = !_this->console_lock;
	_this->button_STOP_PAR.enabled = !_this->console_lock;
	_this->button_STOP_NXM.enabled = !_this->console_lock;
	_this->button_REPEAT.enabled = !_this->console_lock;
	_this->button_FETCH_INST.enabled = !_this->console_lock;
	_this->button_FETCH_DATA.enabled = !_this->console_lock;
	_this->button_WRITE.enabled = !_this->console_lock;
	_this->button_ADDRESS_STOP.enabled = !_this->console_lock;
	_this->button_ADDRESS_BREAK.enabled = !_this->console_lock;
	_this->button_XCT.enabled = !_this->console_lock;

	// SINGLE_INST
	// "Whenever the processor is placed in operation, clear RUN so that it stops at the end of the first instruction.
	// Hence the operator can step through a program one instruction at a time, pressing START for the first one and
	// CONT for subsequent ones. Each time the processor stops, the lights display the same information as when STOP
	// is pressed. Note that read in cannot be done in single instruction mode, as the function' extends over many
	// instructions and there is thus no way to continue."
	// - SINGLE is a mode flag, not a command
	_this->button_SINGLE_INST.enabled = !_this->console_lock;
	single_inst = !!realcons_pdp10_control_get(&_this->button_SINGLE_INST);
	if (single_inst && _this->run_state == RUN_STATE_RUN) {
		// first stop the CPU, like pressing "HALT"
		SIGNAL_SET(cpusignal_console_halt, 1);
		// now START and CONT do single steps
	}

	// STOP pressed: set HALT signal
	_this->button_STOP.enabled = !_this->console_lock && (_this->run_state == RUN_STATE_RUN);
	// "Turn off RUN so the processor stops with STOP MAN on. At the stop PC points to the location of the instruction that will
	// be fetched if CONT is pressed (this is the instruction that would have been done next had the processor not stopped)."
	if (_this->button_STOP.pendingbuttons) {
		SIGNAL_SET(cpusignal_console_halt, 1);
		_this->button_STOP.pendingbuttons = 0;
	}

	// START
	// "Turn on RUN and EXEC MODE KERNEL, and begin normal operation by fetching the instruction at the location specified by
	// address switches 18-35. The memory subroutine for the instruction fetch loads the address into PC for the program
	// to continue. This function does not disturb the flags or the IO equipment."
	_this->button_START.enabled = !_this->console_lock;
	if (_this->button_START.pendingbuttons) {
		// PC value <=?=> value of addr lamps???
		SIGNAL_SET(cpusignal_PC, (int32) realcons_pdp10_control_get(&_this->buttons_ADDRESS));
		if (single_inst) // set PC and single step
			sprintf(_this->realcons->simh_cmd_buffer, "deposit pc %" PRIo64 "\nstep 1\n",
					realcons_pdp10_control_get(&_this->buttons_ADDRESS));
		else
			// SimH start is like
			sprintf(_this->realcons->simh_cmd_buffer, "run %" PRIo64 "\n",
					realcons_pdp10_control_get(&_this->buttons_ADDRESS));
		_this->button_START.pendingbuttons = 0;
	}

	// CONT
	_this->button_CONT.enabled = !_this->console_lock && (_this->run_state != RUN_STATE_RUN);
	if (_this->button_CONT.pendingbuttons) {
		if (single_inst)
			sprintf(_this->realcons->simh_cmd_buffer, "step 1\n");
		else
			sprintf(_this->realcons->simh_cmd_buffer, "cont\n");
		_this->button_CONT.pendingbuttons = 0;
		// runmode setby event "runstart"
	}

	// RESET
	// "Clear all IO devices, disable auto restart, high speed operation and margin programming,
	// clear the processor flags (lighting EXEC MODE KERNEL), turn on the triangular light
	// beside MEMORY DATA (turn off the light beside PROGRAM DATA), turn off RUN and stop the processor.
	_this->button_RESET.enabled = !_this->console_lock;
	if (_this->button_RESET.pendingbuttons) {
		if (_this->run_state == RUN_STATE_RUN) {
			// if machine is running: perform stop, then in stop event do reset
			SIGNAL_SET(cpusignal_console_halt, 1);
		} else {
			sprintf(_this->realcons->simh_cmd_buffer, "reset all\n");
			_this->button_RESET.pendingbuttons = 0;
		}
	}

	// READ IN
	// "Clear all IO devices and all processor flags. Turn on RUN and EXEC MODE KERNEL
	// (trapping and paging will both be disabled as TRAP ENABLE at the top of the console bay will be off).
	// Execute DATAI D,0 where D is the device code specified by the readin device switches at the left end
	// of the maintenance panel (the rightmost device switch is for bit 9 of the instruction and thus
	// selects the least significant octal digit (which is always 0 or 4) in the device code)."
	_this->button_READ_IN.enabled = !_this->console_lock && (_this->run_state != RUN_STATE_RUN);
	if (_this->button_READ_IN.pendingbuttons) {
		unsigned readin_device = (unsigned) realcons_pdp10_control_get(&_this->buttons_READ_IN_DEVICE);
		// readin device: button labels "3..9" -> bits 6..0
		readin_device <<= 2; // bits 1 and 0 are missing
		switch (readin_device) {
		case 0104: // "PTR" paper tape "PTR
			sprintf(_this->realcons->simh_cmd_buffer, "boot ptr\n");
			break;
		case 0320: // "DTA": disk: rp0?
			sprintf(_this->realcons->simh_cmd_buffer, "boot rp0\n");
			break;
		case 0330: // "DTB": disk: rp1?
			sprintf(_this->realcons->simh_cmd_buffer, "boot rp1\n");
			break;
		case 0340: // "MTA": tape TU0
			sprintf(_this->realcons->simh_cmd_buffer, "boot tu0\n");
			break;
		case 0350: // "MTB": tape TU1
			sprintf(_this->realcons->simh_cmd_buffer, "boot tu1\n");
			break;
		default:
			sprintf(_this->realcons->simh_cmd_buffer, "; Unknown boot device: READ-IN DEVICE = %o\n",
					readin_device);
		}
		_this->button_READ_IN.pendingbuttons = 0;
	}

	/* addr/data clear/set
	 * "At the right end of each of these switch registers is a pair of keys that clear or load all the switches in the register
	 * together. The load button sets up the switches according to the contents of the corresponding bits of the memory
	 * indicators (MI) in the fourth row."
	 * So: "address load" loads from DATA LEDs.
	 * LOAD is disabled, if data value was written by program control
	 * Press MI PROG DIS to stop program update and enable LOAD
	 */
	_this->buttons_ADDRESS.enabled = !_this->console_lock;
	_this->button_ADDRESS_CLEAR.enabled = !_this->console_lock;
	_this->button_ADDRESS_LOAD.enabled = !_this->console_lock && !_this->memory_indicator_program;

	_this->buttons_DATA.enabled = !_this->console_datalock;
	_this->button_DATA_CLEAR.enabled = !_this->console_datalock;
	_this->button_DATA_LOAD.enabled = !_this->console_datalock && !_this->memory_indicator_program;
	if (_this->button_ADDRESS_CLEAR.pendingbuttons) {
		realcons_pdp10_control_set(&_this->buttons_ADDRESS, 0);
		_this->button_ADDRESS_CLEAR.pendingbuttons = 0;
	}
	if (_this->button_ADDRESS_LOAD.pendingbuttons) {
		// load addr from data leds
		realcons_pdp10_control_set(&_this->buttons_ADDRESS,
				realcons_pdp10_control_get(&_this->leds_DATA));
		_this->button_ADDRESS_LOAD.pendingbuttons = 0;
	}
	if (_this->button_DATA_CLEAR.pendingbuttons) {
		realcons_pdp10_control_set(&_this->buttons_DATA, 0);
		_this->button_DATA_CLEAR.pendingbuttons = 0;
	}
	if (_this->button_DATA_LOAD.pendingbuttons) {
		// load data from data leds
		realcons_pdp10_control_set(&_this->buttons_DATA,
				realcons_pdp10_control_get(&_this->leds_DATA));
		_this->button_DATA_LOAD.pendingbuttons = 0;
	}

	/* EXAM/DEPOSIT
	 * light "MEMORY DATA" goes on (in feedback event "exam_deposit()"
	 * PAGING SWITCHES:
	 * both off: 22bit absolute physical, else virtual EXEC or USER space (Hiere: always 22 bit)
	 */
	// extra logic: exam/deposit ONLY when machine is stopped
	_this->button_EXAMINE_THIS.enabled = !_this->console_lock
			&& (_this->run_state != RUN_STATE_RUN);
	_this->button_EXAMINE_NEXT.enabled = !_this->console_lock
			&& (_this->run_state != RUN_STATE_RUN);
	_this->button_DEPOSIT_THIS.enabled = !_this->console_lock
			&& (_this->run_state != RUN_STATE_RUN);
	_this->button_DEPOSIT_NEXT.enabled = !_this->console_lock
			&& (_this->run_state != RUN_STATE_RUN);
	if (_this->button_EXAMINE_THIS.pendingbuttons) {
		sprintf(_this->realcons->simh_cmd_buffer, "examine %" PRIo64 "\n",
				realcons_pdp10_control_get(&_this->buttons_ADDRESS));
		// addr&data result written to LEDs in "event_exam_deposit()"
		_this->button_EXAMINE_THIS.pendingbuttons = 0;
	}
	if (_this->button_EXAMINE_NEXT.pendingbuttons) {
		// inc address buttons before examine
		uint64_t addr = realcons_pdp10_control_get(&_this->buttons_ADDRESS);
		addr++;
		sprintf(_this->realcons->simh_cmd_buffer, "examine %" PRIo64 "\n", addr);
		// addr&data result written to LEDs in "event_exam_deposit()"
		_this->button_EXAMINE_NEXT.pendingbuttons = 0;
	}
	if (_this->button_DEPOSIT_THIS.pendingbuttons) {
		sprintf(_this->realcons->simh_cmd_buffer, "deposit %" PRIo64 " %" PRIo64 "\n",
				realcons_pdp10_control_get(&_this->buttons_ADDRESS),
				realcons_pdp10_control_get(&_this->buttons_DATA));
		// addr&data result written to LEDs in "event_exam_deposit()"
		_this->button_DEPOSIT_THIS.pendingbuttons = 0;
	}
	if (_this->button_DEPOSIT_NEXT.pendingbuttons) {
		uint64_t addr = realcons_pdp10_control_get(&_this->buttons_ADDRESS);
		addr++;
		sprintf(_this->realcons->simh_cmd_buffer, "deposit %" PRIo64 " %" PRIo64 "\n", addr,
				realcons_pdp10_control_get(&_this->buttons_DATA));
		// addr&data result written to LEDs in "event_exam_deposit()"
		_this->button_DEPOSIT_NEXT.pendingbuttons = 0;
	}

	// set the MEMORY INDICATOR triangle LEDs above row 4
	if (_this->memory_indicator_program == 1) {
		_this->led_MEMORY_DATA.lamps->value = 0;
		_this->led_PROGRAM_DATA.lamps->value = 1;

	} else {
		_this->led_MEMORY_DATA.lamps->value = 1;
		_this->led_PROGRAM_DATA.lamps->value = 0;
	}

	// DATA Switches set the "console data switch register"
	// (so a Simh "deposit cds ...." is always overwritten with this
	SIGNAL_SET(cpusignal_console_data_switches,
			realcons_pdp10_control_get(&_this->buttons_DATA));

	// RUN/STOP LEDS
	switch (_this->run_state) {
	case RUN_STATE_HALT_MAN:
		// halt bei user
		_this->leds_STOP.lamps->value = 0x4; // STOP.MAN
		_this->led_RUN.lamps->value = 0;
		break;
	case RUN_STATE_HALT_PROG:
		_this->leds_STOP.lamps->value = 0x2; // STOP.PROG
		_this->led_RUN.lamps->value = 0;
		break;
	case RUN_STATE_RUN:
		_this->leds_STOP.lamps->value = 0; // no STOP
		_this->led_RUN.lamps->value = 1;
		break;
	}

	// PI LEDs

	// "The four sets of seven lights at the left display the state of the priority interrupt channels.
	// The PI ACTIVE lights indicate which channels are on. The IOB PI REQUEST lights indicate which
	// channels are receiving request signals over the in-out bus; the PI REQUEST lights indicate
	// channels on which the processor has accepted requests. Except in the case of a program-initiated
	// interrupt, a REQUEST light can go on only if the corresponding ACTIVE light is on. The PI IN
	// PROGRESS lights indicate channels on which interrupts are currently being held; the channel that
	// is actually being serviced is the lowest-numbered one whose light is on. When an IN PROGRESS
	// light goes on, the corresponding REQUEST goes off and cannot go on again until IN PROGRESS goes
	// off when the interrupt is dismissed. PI ON indicates the priority interrupt system is active, so
	// interrupts can be started (this corresponds to CONI PI, bit 28). PI OK 8 indicates that there is
	// no interrupt being held and no channel waiting for an interrupt; this signal is used by the real
	// time clock to discount interrupt time while timing user programs."

	// Different signals names?
	// Simh							KI10 console
	// pi_act "active"				pi in progress  "chanels where interrupts are beeing held"
	// pi_enb "enabled pi levels"	pi active "channels which are ON"
	//
	realcons_pdp10_control_set(&_this->led_PI_ON, !!SIGNAL_GET(cpusignal_pi_on));
	realcons_pdp10_control_set(&_this->leds_PI_ACTIVE, SIGNAL_GET(cpusignal_pi_enb));
	realcons_pdp10_control_set(&_this->leds_PI_IN_PROGRESS, SIGNAL_GET(cpusignal_pi_act));
	realcons_pdp10_control_set(&_this->leds_IOB_PI_REQUEST, SIGNAL_GET(cpusignal_pi_ioq));
	realcons_pdp10_control_set(&_this->leds_PI_REQUEST, SIGNAL_GET(cpusignal_pi_prq));

	// some eye candy
	if (_this->run_state == RUN_STATE_RUN) {
		_this->leds_MODE.lamps->value = 0x01; // kernel exec
	}

	return 0; // OK

}
