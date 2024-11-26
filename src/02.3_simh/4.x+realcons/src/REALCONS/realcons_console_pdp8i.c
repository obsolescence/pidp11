/* realcons_pdp8i.c: Logic for the specific PDP8I panel

   Copyright (c) 2015-2016, Joerg Hoppe
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


   20-Feb-2016  JH      added PANEL_MODE_POWERLESS and power switch logic
   29-Dec-2015  JH      created


   Inputs:
   - realcons_machine state (PDP8 as set by SimH)
   - blinkenlight API panel input (switches)

   Outputs:
   - realcons_machine state (as checked by SimH, example: HALT state)
   - blinkenlight API panel outputs (lamps)
   - SimH command string

   Class hierarchy:
   base panel = generic machine with properties requiered by SimH
  		everthing accessed in scp.c is "base"

   inherited pdp8i (or other architecure)
     an actual Panel has an actual machine state.

   (For instance, a 8/L has almost the same machine state,
   but quite a different panel)

 */
#include <assert.h>

#include "sim_defs.h"
#include "realcons.h"
#include "realcons_console_pdp8i.h"

 // SHORTER macros for signal access
 // assume "realcons_console_logic_pdp8i_t *_this" in context
#define SIGNAL_SET(signal,value) REALCONS_SIGNAL_SET(_this,signal,value)
#define SIGNAL_GET(signal) REALCONS_SIGNAL_GET(_this,signal)



 // indexes of used general timers
#define TIMER_TEST	0
#define TIMER_DATA_FLASH	1

#define TIME_TEST_MS		3000	// self test terminates after 3 seconds
#define TIME_DATA_FLASH_MS	50		// if DATA LEDs flash, they are ON for 1/20 sec
// the RUN LED behaves a bit difficult, so distinguish these states:
#define RUN_STATE_HALT	0
#define RUN_STATE_RESET	1
#define RUN_STATE_WAIT	2
#define RUN_STATE_RUN		3
/*
 * constructor / destructor
 */
realcons_console_logic_pdp8i_t *realcons_console_pdp8i_constructor(realcons_t *realcons)
{
	realcons_console_logic_pdp8i_t *_this;

	_this = (realcons_console_logic_pdp8i_t *)malloc(sizeof(realcons_console_logic_pdp8i_t));
	_this->realcons = realcons; // connect to parent
	_this->run_state = 0;

	return _this;
}

void realcons_console_pdp8i_destructor(realcons_console_logic_pdp8i_t *_this)
{
	free(_this);
}

/*
 * Interface to external simulation:
 * signaled by SimH for state changes of CPU
 * Here used to set DMUX, which is used to drive DATA leds
 *
 * CPU conditions:
 * tbd
 *
 * Console switches:
 * tbd
 * Called high speed in simulator loop!
 * return 0, if not accepted
 */

void realcons_console_pdp8i__event_connect(realcons_console_logic_pdp8i_t *_this)
{
//	realcons_console_clear_output_controls(_this->realcons) ; // every LED to state 0
	// set panel mode to normal
	// On Java panels the powerbutton flips to the ON position.
	realcons_power_mode(_this->realcons, 1);
}

void realcons_console_pdp8i__event_disconnect(realcons_console_logic_pdp8i_t *_this)
{
//	realcons_console_clear_output_controls(_this->realcons); // every LED to state 0
	// set panel mode to "powerless". all lights go off,
	// On Java panels the powerbutton flips to the OFF position
	realcons_power_mode(_this->realcons, 0);
}


void realcons_console_pdp8i__event_opcode_any(realcons_console_logic_pdp8i_t *_this)
{
#ifdef TODO
	// other opcodes executed by processor
	if (_this->realcons->debug)
		printf("realcons_console_pdp8i__event_opcode_any\n");
	// after any opcode: ADDR shows PC, DATA shows IR = opcode
	SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_pdp11.PC));
	_this->DMUX = SIGNAL_GET(signals_cpu_pdp11.instruction_register);
	// show opcode
	_this->led_BUS->value = 1; // UNIBUS cycles ....
	_this->led_PROCESSOR->value = 1; // ... by processor
	_this->run_state = RUN_STATE_RUN;
#endif
}

void realcons_console_pdp8i__event_opcode_halt(realcons_console_logic_pdp8i_t *_this)
{
#ifdef TODO
	if (_this->realcons->debug)
		printf("realcons_console_pdp8i__event_opcode_halt\n");
	SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_pdp11.PC));
	_this->DMUX = SIGNAL_GET(signals_cpu_pdp11.R0);
	_this->led_BUS->value = 0; // no UNIBUS cycles any more
	_this->led_PROCESSOR->value = 0; // processor stop
	_this->run_state = RUN_STATE_HALT;
#endif
}

void realcons_console_pdp8i__event_opcode_reset(realcons_console_logic_pdp8i_t *_this)
{
#ifdef TODO
	if (_this->realcons->debug)
		printf("realcons_console_pdp8i__event_opcode_reset\n");
	SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_pdp11.PC));
	_this->DMUX = SIGNAL_GET(signals_cpu_pdp11.R0);
	_this->led_BUS->value = 1; // http://www.youtube.com/watch?v=iIsZVqhaneo
	_this->led_PROCESSOR->value = 1;
	_this->run_state = RUN_STATE_RESET;
#endif
}

void realcons_console_pdp8i__event_opcode_wait(realcons_console_logic_pdp8i_t *_this)
{
#ifdef TODO
	if (_this->realcons->debug)
		printf("realcons_console_pdp8i__event_opcode_wait\n");
	SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_pdp11.PC));
	_this->DMUX = SIGNAL_GET(signals_cpu_pdp11.instruction_register);
	_this->led_BUS->value = 0; // no UNIBUS cycles any more
	_this->led_PROCESSOR->value = 0; // processor idle
	_this->run_state = RUN_STATE_WAIT; // RUN led off
#endif
}

void realcons_console_pdp8i__event_run_start(realcons_console_logic_pdp8i_t *_this)
{
#ifdef TODO
	if (_this->realcons->debug)
		printf("realcons_console_pdp8i__event_run_start\n");
	if (_this->switch_HALT->value)
		return; // do not accept RUN command
	// set property RUNMODE, so SimH can read it back
	_this->led_BUS->value = 1; // start accessing UNIBUS cycles
	_this->led_PROCESSOR->value = 1; // processor active
	_this->run_state = RUN_STATE_RUN;
#endif
}

void realcons_console_pdp8i__event_step_start(realcons_console_logic_pdp8i_t *_this)
{
#ifdef TODO
	if (_this->realcons->debug)
		printf("realcons_console_pdp8i__event_step_start\n");
	_this->led_BUS->value = 1; // start accessing UNIBUS cycles
	_this->led_PROCESSOR->value = 1; // processor active
#endif
}

// after single step and on operator halt the same happens
void realcons_console_pdp8i__event_operator_step_halt(realcons_console_logic_pdp8i_t *_this)
{
#ifdef TODO
	if (_this->realcons->debug)
		printf("realcons_console_pdp8i__event_operator_step_halt\n");
	SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_pdp11.PC));
	_this->DMUX = SIGNAL_GET(signals_cpu_pdp11.PSW);
	_this->led_BUS->value = 0; // no UNIBUS cycles any more
	_this->led_PROCESSOR->value = 0; // processor idle
	_this->run_state = RUN_STATE_HALT;
#endif
}
void realcons_console_pdp8i__event_operator_exam_deposit(realcons_console_logic_pdp8i_t *_this)
{
	int32 tmp_pc;
	if (_this->realcons->debug)
		printf("realcons_console_pdp8i__event_operator_exam_deposit\n");
	// same code as operating the EXAM/DEP switch: Update PC to mem-addr+1
	tmp_pc = SIGNAL_GET(cpusignal_memory_address_phys_register);
	if ((tmp_pc & 0xfff) == 0xfff)
		tmp_pc &= 0xf000; // roll around lower 12 bits
	else tmp_pc++; // inc inside page
	SIGNAL_SET(cpusignal_if_pc, tmp_pc);

	// exam on SimH console sets also console address (like LOAD ADR)
//	SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_generic.bus_address_register));
//	_this->DMUX = SIGNAL_GET(signals_cpu_generic.bus_data_register);
//	_this->led_BUS->value = 0; // no UNIBUS cycles any more
//	_this->led_PROCESSOR->value = 0; // processor idle

}


void realcons_console_pdp8i_interface_connect(realcons_console_logic_pdp8i_t *_this,
	realcons_console_controller_interface_t *intf, char *panel_name)
{
	intf->name = panel_name;
	intf->constructor_func =
		(console_controller_constructor_func_t)realcons_console_pdp8i_constructor;
	intf->destructor_func =
		(console_controller_destructor_func_t)realcons_console_pdp8i_destructor;
	intf->reset_func = (console_controller_reset_func_t)realcons_console_pdp8i_reset;
	intf->service_func = (console_controller_service_func_t)realcons_console_pdp8i_service;
	intf->test_func = (console_controller_test_func_t)realcons_console_pdp8i_test;
	intf->event_connect = (console_controller_event_func_t)realcons_console_pdp8i__event_connect;
	intf->event_disconnect = (console_controller_event_func_t)realcons_console_pdp8i__event_disconnect;

	// connect pdp8 cpu signals end events to simulator and realcons state variables
	{
		// REALCONS extension in scp.c
		extern t_addr realcons_memory_address_phys_register; // REALCONS extension in scp.c
		extern char *realcons_register_name; // pseudo: name of last accessed register
		extern t_value realcons_memory_data_register; // REALCONS extension in scp.c
		extern  int realcons_console_halt; // 1: CPU halted by realcons console
		extern volatile t_bool sim_is_running; // global in scp.c

		extern int32 SR; /* switch register, global in cpu_pdp8.c */
		extern int32 saved_LAC;									/* saved L'AC */
		extern int32 saved_MQ; 									/* saved MQ */
		extern int32 saved_PC; 									/* saved IF'PC */
		extern int32 saved_DF; 									/* saved Data Field */
		extern int32 SC; 										/* Step counter */

		extern int32 realcons_instruction_decode; // bit mask for decoded instruction, see REALCONS_PDP8I_INSTRUCTION_DECODE_*
		extern int32 realcons_majorstate_curinstr; // all states of current instruction
		extern int32 realcons_majorstate_last; // last active state

		extern int32 realcons_flipflops; // see REALCONS_PDP8I_FLIFLOPE_*

		realcons_console_halt = 0;
		// is "sim_is_running" indeed identical with our "cpu_is_running" ?
		// may cpu stops, but some device are still serviced?
		_this->cpusignal_run = &sim_is_running;

		// from scp.c
		_this->cpusignal_memory_address_phys_register = &realcons_memory_address_phys_register;
        _this->cpusignal_register_name = &realcons_register_name; // pseudo: name of last accessed register
		_this->cpusignal_memory_data_register = &realcons_memory_data_register;
		_this->cpusignal_console_halt = &realcons_console_halt;
		// from pdp8_cpu.c
		_this->cpusignal_switch_register = &SR; // see pdp8_cpu.c
		_this->cpusignal_if_pc = &saved_PC; // IF'PC, 15bit PC
		_this->cpusignal_df = &saved_DF;
		_this->cpusignal_link_accumulator = &saved_LAC;
		_this->cpusignal_multiplier_quotient = &saved_MQ;
		_this->cpusignal_step_counter = &SC;

		_this->cpusignal_instruction_decode = &realcons_instruction_decode;
		// bit mask for decoded instruction, see REALCONS_PDP8I_INSTRUCTION_DECODE_*

		//
		_this->cpusignal_majorstate_curinstr = &realcons_majorstate_curinstr;
		_this->cpusignal_majorstate_last = &realcons_majorstate_last;
		_this->cpusignal_flipflops = &realcons_flipflops;

	}

	/*** link handler to cpu/device events ***/
	{
		// scp.c
		extern console_controller_event_func_t realcons_event_run_start;
		extern console_controller_event_func_t realcons_event_operator_halt;
		extern console_controller_event_func_t realcons_event_step_start;
		extern console_controller_event_func_t realcons_event_step_halt;
		extern console_controller_event_func_t realcons_event_operator_exam;
		extern console_controller_event_func_t realcons_event_operator_deposit;
		extern console_controller_event_func_t realcons_event_operator_reg_exam;
		extern console_controller_event_func_t realcons_event_operator_reg_deposit;
		extern console_controller_event_func_t realcons_event_cpu_reset;

		realcons_event_run_start =
			(console_controller_event_func_t)realcons_console_pdp8i__event_run_start;
		realcons_event_operator_halt = NULL;
		realcons_event_step_start =
			(console_controller_event_func_t)realcons_console_pdp8i__event_step_start;
		realcons_event_step_halt =
			(console_controller_event_func_t)realcons_console_pdp8i__event_operator_step_halt;
		realcons_event_operator_exam =
			realcons_event_operator_deposit =
			(console_controller_event_func_t)realcons_console_pdp8i__event_operator_exam_deposit;
		realcons_event_operator_reg_exam = realcons_event_operator_reg_deposit = NULL ;
		realcons_event_cpu_reset = NULL ;
	}
#ifdef TODO
	{
		// set event ptrs in pdp8_cpu.c
		extern console_controller_event_func_t realcons_event_opcode_halt;
		//			_this->realcons->events_cpu_generic.opcode_any =
		//				(console_controller_event_func_t)realcons_console_pdp8i__event_opcode_any;
		realcons_event_opcode_halt = (console_controller_event_func_t)realcons_console_pdp8i__event_opcode_halt;
	}
#endif
}

// setup first state
t_stat realcons_console_pdp8i_reset(realcons_console_logic_pdp8i_t *_this)
{
	_this->realcons->simh_cmd_buffer[0] = '\0';
	// The 8i console has no addr register on its own, the CPU memory_address_register is used.

	/*
	 * direct links to all required controls.
	 * See PiDP8 Blinkenlight API server project. Control names exactly like printed onto the PDP8/I acryl
	 */
    if (!(_this->keyswitch_power = realcons_console_get_input_control(_this->realcons, "POWER")))
        return SCPE_NOATT;
    if (!(_this->keyswitch_panel_lock = realcons_console_get_input_control(_this->realcons, "PANEL LOCK")))
        return SCPE_NOATT;

    if (!(_this->switch_start = realcons_console_get_input_control(_this->realcons, "Start")))
		return SCPE_NOATT;
	if (!(_this->switch_load_address = realcons_console_get_input_control(_this->realcons, "Load Add")))
		return SCPE_NOATT;
	if (!(_this->switch_deposit = realcons_console_get_input_control(_this->realcons, "Dep")))
		return SCPE_NOATT;
	if (!(_this->switch_examine = realcons_console_get_input_control(_this->realcons, "Exam")))
		return SCPE_NOATT;
	if (!(_this->switch_continue = realcons_console_get_input_control(_this->realcons, "Cont")))
		return SCPE_NOATT;
	if (!(_this->switch_stop = realcons_console_get_input_control(_this->realcons, "Stop")))
		return SCPE_NOATT;
	if (!(_this->switch_single_step = realcons_console_get_input_control(_this->realcons, "Sing Step")))
		return SCPE_NOATT;
	if (!(_this->switch_single_instruction = realcons_console_get_input_control(_this->realcons, "Sing Inst")))
		return SCPE_NOATT;
	if (!(_this->switch_switch_register = realcons_console_get_input_control(_this->realcons, "SR")))
		return SCPE_NOATT;
	if (!(_this->switch_data_field = realcons_console_get_input_control(_this->realcons, "DF")))
		return SCPE_NOATT;
	if (!(_this->switch_instruction_field = realcons_console_get_input_control(_this->realcons, "IF")))
		return SCPE_NOATT;

	if (!(_this->led_program_counter = realcons_console_get_output_control(_this->realcons, "Program Counter")))
		return SCPE_NOATT;
	if (!(_this->led_inst_field = realcons_console_get_output_control(_this->realcons, "Inst Field")))
		return SCPE_NOATT;
	if (!(_this->led_data_field = realcons_console_get_output_control(_this->realcons, "Data Field")))
		return SCPE_NOATT;
	if (!(_this->led_memory_address = realcons_console_get_output_control(_this->realcons, "Memory Address")))
		return SCPE_NOATT;
	if (!(_this->led_memory_buffer = realcons_console_get_output_control(_this->realcons, "Memory Buffer")))
		return SCPE_NOATT;
	if (!(_this->led_accumulator = realcons_console_get_output_control(_this->realcons, "Accumulator")))
		return SCPE_NOATT;
	if (!(_this->led_link = realcons_console_get_output_control(_this->realcons, "Link")))
		return SCPE_NOATT;
	if (!(_this->led_multiplier_quotient = realcons_console_get_output_control(_this->realcons, "Multiplier Quotient")))
		return SCPE_NOATT;
	if (!(_this->led_and = realcons_console_get_output_control(_this->realcons, "And")))
		return SCPE_NOATT;
	if (!(_this->led_tad = realcons_console_get_output_control(_this->realcons, "Tad")))
		return SCPE_NOATT;
	if (!(_this->led_isz = realcons_console_get_output_control(_this->realcons, "Isz")))
		return SCPE_NOATT;
	if (!(_this->led_dca = realcons_console_get_output_control(_this->realcons, "Dca")))
		return SCPE_NOATT;
	if (!(_this->led_jms = realcons_console_get_output_control(_this->realcons, "Jms")))
		return SCPE_NOATT;
	if (!(_this->led_jmp = realcons_console_get_output_control(_this->realcons, "Jmp")))
		return SCPE_NOATT;
	if (!(_this->led_iot = realcons_console_get_output_control(_this->realcons, "Iot")))
		return SCPE_NOATT;
	if (!(_this->led_opr = realcons_console_get_output_control(_this->realcons, "Opr")))
		return SCPE_NOATT;
	if (!(_this->led_fetch = realcons_console_get_output_control(_this->realcons, "Fetch")))
		return SCPE_NOATT;
	if (!(_this->led_execute = realcons_console_get_output_control(_this->realcons, "Execute")))
		return SCPE_NOATT;
	if (!(_this->led_defer = realcons_console_get_output_control(_this->realcons, "Defer")))
		return SCPE_NOATT;
	if (!(_this->led_word_count = realcons_console_get_output_control(_this->realcons, "Word Count")))
		return SCPE_NOATT;
	if (!(_this->led_current_address = realcons_console_get_output_control(_this->realcons, "Current Address")))
		return SCPE_NOATT;
	if (!(_this->led_break = realcons_console_get_output_control(_this->realcons, "Break")))
		return SCPE_NOATT;
	if (!(_this->led_ion = realcons_console_get_output_control(_this->realcons, "Ion")))
		return SCPE_NOATT;
	if (!(_this->led_pause = realcons_console_get_output_control(_this->realcons, "Pause")))
		return SCPE_NOATT;
	if (!(_this->led_run = realcons_console_get_output_control(_this->realcons, "Run")))
		return SCPE_NOATT;
	if (!(_this->led_step_counter = realcons_console_get_output_control(_this->realcons, "Step Counter")))
		return SCPE_NOATT;


	// verify: is the power switch turned ON?

	return SCPE_OK;
}

// process panel state.
// operates on Blinkenlight_API panel structs,
// but RPC communication is done external
t_stat realcons_console_pdp8i_service(realcons_console_logic_pdp8i_t *_this)
{
	blinkenlight_panel_t *p = _this->realcons->console_model; // alias

	int console_mode;
	unsigned	sing_inst_active;

	blinkenlight_control_t *action_switch; // current action switch

	/* test time expired? */

	// STOP by activating HALT?
	if (_this->switch_stop->value && !SIGNAL_GET(cpusignal_console_halt)) {
		// must be done by SimH.pdp8_cpu.c!
	}


	// CONSOLE: processor accepts commands from console panel when HALTed
	console_mode = !SIGNAL_GET(cpusignal_run)
		|| SIGNAL_GET(cpusignal_console_halt);

	/*************************************************************
	 * eval switches
	 * react on changed action switches
	 */

	 // fetch switch register
	SIGNAL_SET(cpusignal_switch_register, _this->switch_switch_register->value);

	if (_this->keyswitch_power->value == 0 && _this->keyswitch_power->value_previous == 1) {
		// Power switch transition to POWER OFF: terminate SimH
		// This is drastic, but will teach users not to twiddle with the power switch.
		// when panel is disconnected, panel mode goes to POWERLESS and power switch goes OFF.
		// But shutdown sequence is not initiated, because we're disconnected then.
		SIGNAL_SET(cpusignal_console_halt, 1); // stop execution
		sprintf(_this->realcons->simh_cmd_buffer, "quit"); // do not confirm the quit with ENTER
		return SCPE_OK;
	}



	if (_this->keyswitch_panel_lock->value == 0) {
		/*
		 * Panel Lock key switch:   When turned clockwise, this key - operated switch disables
		 *	all controls except the Switch Register switches on the
		 *	operator console.In this condition, inadvertent key
		 *	operation cannot disturb the program.The program can,
		 *	however, monitor the content of SR by execution of the
		 *	OSR instruction.*/

		 // fetch HALT mode, must be sensed by simulated processor to produce state OPERATOR_HALT
		SIGNAL_SET(cpusignal_console_halt, _this->switch_stop->value);

		/* These are "momentary contact" switches:
		 * Start, Exam, Load Add, Cont, Dep, Stop
		 * See Maintenance Manual PDP-8I Volume I (Jul 1969, DEC-8I-HR1A-D)_chapter_3.pdf
		 * Which momentary caontact switch was activated? Process only one of these */
		action_switch = NULL;
		if (!action_switch && _this->switch_load_address->value == 1
			&& _this->switch_load_address->value_previous == 0)
			action_switch = _this->switch_load_address;
		if (!action_switch && _this->switch_examine->value == 1 && _this->switch_examine->value_previous == 0)
			action_switch = _this->switch_examine;
		if (!action_switch && _this->switch_deposit->value == 1
			&& _this->switch_deposit->value_previous == 0)
			action_switch = _this->switch_deposit;
		if (!action_switch && _this->switch_continue->value == 1 && _this->switch_continue->value_previous == 0)
			action_switch = _this->switch_continue;
		if (!action_switch && _this->switch_start->value == 1 && _this->switch_start->value_previous == 0)
			action_switch = _this->switch_start;
		// HALT already processed, see above

		// first: reset "switch changed" condition
		if (action_switch)
			action_switch->value_previous = action_switch->value;

		/*
		 * ATTENTION:
		 * Switch actions may NOT manipulate the console state directly!
		 * They ONLY may generated SimH commands,
		 * which in turn lets SimH signal a changed machine state
		 * which is evaluated for display of panel indicators
		 */


		 //  PC is used by LOAD AD, Exam and Dep!
		 //  After EXAM/DEP: PC is incremented. MA shows current address
		 //      aus

		 // accept input only in CONSOLE mode. Exception: HALT is always enabled, see above
		if (!console_mode)
			action_switch = NULL;


		if (action_switch == _this->switch_load_address) {
			/* "This key transfers the content of SR into PC, the content
			 * of INST FIELD switches into IF, the content of the DATA
			 * FIELD switches into DF, and clears the major state
			 * flip-flops." */
			SIGNAL_SET(cpusignal_if_pc,
				((_this->switch_instruction_field->value & 7) << 12)
				| _this->switch_switch_register->value & 0x0fff);
			SIGNAL_SET(cpusignal_df, _this->switch_data_field->value & 7);
			SIGNAL_SET(cpusignal_majorstate_curinstr, 0); // clear major state flip flops
			SIGNAL_SET(cpusignal_majorstate_last, 0); // clear major state flip flops

			if (_this->realcons->debug)
				printf("LOADADR %o\n", SIGNAL_GET(cpusignal_memory_address_phys_register));
		}

		if (action_switch == _this->switch_examine) {
			/* "This key transfers the content of core memory at the address
			 * specified by the content of PC, into the MB. The content
			 * of the PC is then incremented by one to allow
			 * examination of the contents of sequential core memory
			 * addresses by repeated operation of the Exam key. The
			 * major state flip - flop register cleared.The MA indicates
			 * the address of the data in the MB." */
			int32	tmp_pc;

			// generate simh "exam cmd"
			// fix octal, should use SimH-radix
			sprintf(_this->realcons->simh_cmd_buffer, "examine %o\n", SIGNAL_GET(cpusignal_if_pc));

			// "inc by one": rollaround in 12 bit PC space
			tmp_pc = SIGNAL_GET(cpusignal_if_pc);
			if ((tmp_pc & 0xfff) == 0xfff)
				tmp_pc &= 0xf000; // roll around lower 12 bits
			else tmp_pc++; // inc inside page
			SIGNAL_SET(cpusignal_if_pc, tmp_pc);
			SIGNAL_SET(cpusignal_majorstate_curinstr, 0); // clear major state flip flops
			SIGNAL_SET(cpusignal_majorstate_last, 0); // clear major state flip flops
												 // SimH executes "exam", which should set MA,MB over
												// signals_cpu_generic.bus_address|data_register
		}

		if (action_switch == _this->switch_deposit) {
			/* "This key transfers the content of SR into MB and core memory
			 * at the address specified by the current content of PC.
			 * The major state flip-flops are cleared. The contents of
			 * PC is then incremented by one to allow storing of
			 * information in sequential core memory addresses by
			 * repeated operation of the Dep key." */
			int32	tmp_pc;
			unsigned dataval;
			dataval = (realcons_machine_word_t)_this->switch_switch_register->value & 0x0fff; // trunc to switches 11..0

			// produce SimH cmd. fix octal, should use SimH-radix
			sprintf(_this->realcons->simh_cmd_buffer, "deposit %o %o\n",
				SIGNAL_GET(cpusignal_if_pc), dataval);

			tmp_pc = SIGNAL_GET(cpusignal_if_pc);
			if ((tmp_pc & 0xfff) == 0xfff)
				tmp_pc &= 0xf000; // roll around lower 12 bits
			else tmp_pc++; // inc inside page
			SIGNAL_SET(cpusignal_if_pc, tmp_pc);
			SIGNAL_SET(cpusignal_majorstate_curinstr, 0); // clear major state flip flops
			SIGNAL_SET(cpusignal_majorstate_last, 0); // clear major state flip flops
			// SimH executes "deposit", which should set MA,MB over
			// signals_cpu_generic.bus_address|data_register
		}

		// STOP already processed, see above


			/* "This key allows execution of one instruction. When the
			* computer is started by pressing the Start or Cont key,
			* the Sing Inst key causes the RUN flip - flop to be cleared
			* at the end of the last cycle of the current instruction.
			* Thereafter, repeated operation of the Cont key steps the
			* program one instruction at a time." */
		sing_inst_active = !!_this->switch_single_instruction->value;


		/* "This key causes the RUN flip - flop to be cleared to disable
			* the timing circuits at the end of one cycle of
			* operation. Thereafter, repeated operation of the Cont
			* key steps the program one cycle at a time so that the
			* operator can observe the contents of registers in each
			* major state." */
			// "Sing Step" not implemented


		if (action_switch == _this->switch_start) {
			/* rising edge of START
			 * "Starts the program by turning off the program interrupt
			 * circuits clearing the AC and L, setting the Fetch state,
			 * and starts the central processor." */
			if (sing_inst_active) {
				sprintf(_this->realcons->simh_cmd_buffer, "deposit pc %o\nstep 1\n",
					SIGNAL_GET(cpusignal_if_pc));
			}
			else
				sprintf(_this->realcons->simh_cmd_buffer, "run %o\n",
					SIGNAL_GET(cpusignal_if_pc));
		}
		else if (action_switch == _this->switch_continue) {

			/* Rising edge of CONT
			 * " This key sets the RUN flip-flop to continue the program
			 * in the state and instruction designated by the lighted
			 * console indicators, at the address currently specified
			 * by the PC if key SS is not on." */
			if (sing_inst_active)
				sprintf(_this->realcons->simh_cmd_buffer, "step 1\n");
			else
				sprintf(_this->realcons->simh_cmd_buffer, "cont\n");
		}
	} // if ! key lock



	/***************************************************
	 * update lights
	 */
	 // while test not expired: do lamp test
	if (!_this->realcons->lamp_test && _this->realcons->timer_running_msec[TIMER_TEST])
		realcons_lamp_test(_this->realcons, 1); // begin lamptest
	if (_this->realcons->lamp_test && !_this->realcons->timer_running_msec[TIMER_TEST])
		realcons_lamp_test(_this->realcons, 0); // end lamptest
	if (_this->realcons->lamp_test)
		return SCPE_OK; // no lights need to be set

	if (!_this->realcons->lamp_test) {

		// ADRESS always shows BUS Adress register.
		_this->led_memory_address->value = SIGNAL_GET(cpusignal_memory_address_phys_register);
		//	if (_this->realcons->debug)
		//		printf("led_ADDRESS=%o, ledDATA=%o\n", (unsigned) _this->led_ADDRESS->value,
		//				(unsigned) _this->led_DATA->value);
		_this->led_memory_buffer->value = SIGNAL_GET(cpusignal_memory_data_register);

		_this->led_inst_field->value = SIGNAL_GET(cpusignal_if_pc) >> 12;
		_this->led_program_counter->value = SIGNAL_GET(cpusignal_if_pc) & 07777;
		_this->led_data_field->value = SIGNAL_GET(cpusignal_df);

		//// "Accumulator  Indicates the content of AC."
		_this->led_accumulator->value = SIGNAL_GET(cpusignal_link_accumulator) & 07777;
		// "Link Indicates the content of L."
		_this->led_link->value = !!(SIGNAL_GET(cpusignal_link_accumulator) >> 12);

		//	"Multiplier Quotient     Indicates the content of the multiplier quotient(MQ).
		//	MQ holds the multiplier at the beginning of a
		//	multiplication and holds the least - significant half of
		//	the product at the conclusion.It holds the
		//	least - significant half of the dividend at the start of
		//	division and holds the quotient at the conclusion.
		_this->led_multiplier_quotient->value = SIGNAL_GET(cpusignal_multiplier_quotient);

		_this->led_step_counter->value = SIGNAL_GET(cpusignal_step_counter);

		// Major state indicators
		{
			// if cpu is running, display of all states active in current instruction.
			// This is incorrect, but instruction execution is atomic in
			// SimH, no sumlation on major state level.
			// If cpu is stopped, display last active state (historic correct).
			int	majorstate;
			if (SIGNAL_GET(cpusignal_flipflops) & REALCONS_PDP8I_FLIPFLOP_RUN)
				majorstate = SIGNAL_GET(cpusignal_majorstate_curinstr);
			else
				majorstate = SIGNAL_GET(cpusignal_majorstate_last);
			_this->led_fetch->value = !!(majorstate & REALCONS_PDP8I_MAJORSTATE_FETCH);
			_this->led_execute->value = !!(majorstate & REALCONS_PDP8I_MAJORSTATE_EXECUTE);
			_this->led_defer->value = !!(majorstate & REALCONS_PDP8I_MAJORSTATE_DEFER);
			_this->led_word_count->value = !!(majorstate & REALCONS_PDP8I_MAJORSTATE_WORDCOUNT);
			_this->led_current_address->value = !!(majorstate & REALCONS_PDP8I_MAJORSTATE_CURRENTADDRESS);
			_this->led_break->value = !!(majorstate & REALCONS_PDP8I_MAJORSTATE_BREAK);
		}

		// Miscellaneous indicators
		_this->led_ion->value = !!(SIGNAL_GET(cpusignal_flipflops) & REALCONS_PDP8I_FLIPFLOP_ION);
		_this->led_pause->value = !!(SIGNAL_GET(cpusignal_flipflops) & REALCONS_PDP8I_FLIPFLOP_PAUSE);
		_this->led_run->value = !!(SIGNAL_GET(cpusignal_flipflops) & REALCONS_PDP8I_FLIPFLOP_RUN);
		// alternative, PDP-11 like
		// _this->led_run->value = !!(SIGNAL_GET(cpusignal_run);


	// Instruction indicators
		_this->led_and->value = !!(SIGNAL_GET(cpusignal_instruction_decode) & REALCONS_PDP8I_INSTRUCTION_DECODE_AND);
		_this->led_tad->value = !!(SIGNAL_GET(cpusignal_instruction_decode) & REALCONS_PDP8I_INSTRUCTION_DECODE_TAD);
		_this->led_isz->value = !!(SIGNAL_GET(cpusignal_instruction_decode) & REALCONS_PDP8I_INSTRUCTION_DECODE_ISZ);
		_this->led_dca->value = !!(SIGNAL_GET(cpusignal_instruction_decode) & REALCONS_PDP8I_INSTRUCTION_DECODE_DCA);
		_this->led_jms->value = !!(SIGNAL_GET(cpusignal_instruction_decode) & REALCONS_PDP8I_INSTRUCTION_DECODE_JMS);
		_this->led_jmp->value = !!(SIGNAL_GET(cpusignal_instruction_decode) & REALCONS_PDP8I_INSTRUCTION_DECODE_JMP);
		_this->led_iot->value = !!(SIGNAL_GET(cpusignal_instruction_decode) & REALCONS_PDP8I_INSTRUCTION_DECODE_IOT);
		_this->led_opr->value = !!(SIGNAL_GET(cpusignal_instruction_decode) & REALCONS_PDP8I_INSTRUCTION_DECODE_OPR);
	} // if ! lamptest

	return SCPE_OK;
}

/*
 * start 1 sec test sequence.
 * - lamps ON for 1 sec
 * - print state of all switches
 */
int realcons_console_pdp8i_test(realcons_console_logic_pdp8i_t *_this, int arg)
{
	// send end time for test: 1 second = curtime + 1000
	// lamp test is set in service()
	_this->realcons->timer_running_msec[TIMER_TEST] = _this->realcons->service_cur_time_msec
		+ TIME_TEST_MS;

	realcons_printf(_this->realcons, stdout, "Verify lamp test!\n");
	realcons_printf(_this->realcons, stdout, "Switch 'Data Field' = %llo\n", _this->switch_data_field->value);
	realcons_printf(_this->realcons, stdout, "Switch 'Inst Field' = %llo\n", _this->switch_instruction_field->value);
	realcons_printf(_this->realcons, stdout, "Switch Register     = %llo\n", _this->switch_switch_register->value);
	realcons_printf(_this->realcons, stdout, "Switch 'Start'      = %llo\n",
		_this->switch_start->value);
	realcons_printf(_this->realcons, stdout, "Switch 'Load Add'   = %llo\n",
		_this->switch_load_address->value);
	realcons_printf(_this->realcons, stdout, "Switch 'Dep'        = %llo\n",
		_this->switch_deposit->value);
	realcons_printf(_this->realcons, stdout, "Switch 'Exam'       = %llo\n",
		_this->switch_examine->value);
	realcons_printf(_this->realcons, stdout, "Switch 'Cont'       = %llo\n",
		_this->switch_continue->value);
	realcons_printf(_this->realcons, stdout, "Switch 'Stop'       = %llo\n",
		_this->switch_stop->value);
	realcons_printf(_this->realcons, stdout, "Switch 'Sing Step'  = %llo\n",
		_this->switch_single_step->value);
	realcons_printf(_this->realcons, stdout, "Switch 'Sing Inst'  = %llo\n",
		_this->switch_single_instruction->value);
	return 0; // OK
}
