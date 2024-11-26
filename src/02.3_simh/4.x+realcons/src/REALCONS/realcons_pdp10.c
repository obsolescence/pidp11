
/*  realcons_pdp10.c: State and function identical for the PDP10 KA10 and KI10 console

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

   20-Feb-2016  JH      added PANEL_MODE_POWERLESS and power switch logic
   31-Aug-2014  JH      created


   Signals into simlated CPU.
   extensions to the simulated cpu.

   Life cycle of state:
	generated on SimH simulator start
	destroyed: never.
 */

#define REALCONS_PDP10_C_	// enable private global definitions in realcons_cpu_pdp10.h
#include	"realcons.h"
#include	"realcons_pdp10.h"
#include	"realcons_pdp10_control.h"



 /*
  * constructor / destructor
  */
realcons_console_logic_pdp10_t *realcons_console_pdp10_constructor(realcons_t *realcons)
{
	realcons_console_logic_pdp10_t *_this;

	_this = (realcons_console_logic_pdp10_t *)malloc(sizeof(realcons_console_logic_pdp10_t));
	_this->realcons = realcons; // connect to parent
	_this->run_state = 0;
	return _this;
}

void realcons_console_pdp10_destructor(realcons_console_logic_pdp10_t *_this)
{
	free(_this);
}

void realcons_console_pdp10_event_connect(realcons_console_logic_pdp10_t *_this)
{
	realcons_console_pdp10_reset(_this); // everything off

	// panel connected to SimH: "POWER" Led, hour meter
	_this->led_POWER.lamps->value = 1;
	_this->enable_HOURMETER.lamps->value = 1; // "lamps" = output :-/

	// Set panel mode to normal
	 // On Java panels the powerbutton flips to the ON position.
	realcons_power_mode(_this->realcons, 1);
}

void realcons_console_pdp10_event_disconnect(realcons_console_logic_pdp10_t *_this)
{
	realcons_console_pdp10_reset(_this); // everything off

	// panel disconnected from SimH: "POWER" Led
	_this->led_POWER.lamps->value = 0;
	_this->enable_HOURMETER.lamps->value = 0; // "lamps" = output :-/

  // Set panel mode to normal
 // On Java panels the powerbutton flips to the ON position.
	realcons_power_mode(_this->realcons, 0);
}

void realcons_console_pdp10__event_opcode_any(realcons_console_logic_pdp10_t *_this)
{
	// other opcodes executed by processor
	if (_this->realcons->debug)
		printf("realcons_console_pdp10__event_opcode_any\n");

	// set PC and instruction LEDS
	realcons_pdp10_control_set(&_this->leds_PROGRAM_COUNTER, SIGNAL_GET(cpusignal_PC));
	realcons_pdp10_control_set(&_this->leds_INSTRUCTION, SIGNAL_GET(cpusignal_instruction));

	/*
	 // SIGNAL_SET(signals_cpu_generic.cpu_is_running, 1);
	 // after any opcode: ADDR shows PC, DATA shows IR = opcode
	 SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_pdp11.PC));
	 // opcode fetch: ALU_output already set
	 SIGNAL_SET(signals_cpu_generic.bus_data_register, SIGNAL_GET(signals_cpu_pdp11.instruction_register));
	 // SIGNAL_SET(signals_cpu_pdp11.bus_register, SIGNAL_GET(signals_cpu_pdp11.instruction_register));
	 _this->led_MASTER->value = 1; // processor fetches, is unibus master
	 _this->led_PAUSE->value = 0; // see 1.3.4
	 */
	 // _this->run_state = RUN_STATE_RUN; // single step?
}

void realcons_console_pdp10__event_opcode_halt(realcons_console_logic_pdp10_t *_this)
{
	if (_this->realcons->debug)
		printf("realcons_console_pdp10__event_opcode_halt\n");
	SIGNAL_SET(cpusignal_console_halt, 0);
	/*
	 SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_pdp11.PC));
	 // what is in the unibus interface on HALT? IR == HALT Opcode?
	 // SIGNAL_SET(signals_cpu_pdp11.busregister, ...)
	 // _this->DMUX = SIGNAL_GET(signals_cpu_pdp11.R0);
	 _this->led_MASTER->value = 0; // sure ?
	 _this->led_PAUSE->value = 0; // see 1.3.4
	 */
	_this->run_state = RUN_STATE_HALT_PROG;
}

void realcons_console_pdp10__event_opcode_reset(realcons_console_logic_pdp10_t *_this)
{
	if (_this->realcons->debug)
		printf("realcons_console_pdp10__event_opcode_reset\n");
	SIGNAL_SET(cpusignal_console_halt, 0);
	/*
	 SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_pdp11.PC));
	 // what is in the unibus interface on RESET? IR == RESET Opcode ?
	 // SIGNAL_SET(signals_cpu_pdp11.busregister, ...)
	 // _this->DMUX = SIGNAL_GET(signals_cpu_pdp11.R0);
	 _this->led_MASTER->value = 0; // sure ?
	 _this->led_PAUSE->value = 0; // see 1.3.4
	 _this->run_state = RUN_STATE_RESET;
	 */
}

void realcons_console_pdp10__event_opcode_wait(realcons_console_logic_pdp10_t *_this)
{
	if (_this->realcons->debug)
		printf("realcons_console_pdp10__event_opcode_wait\n");
	SIGNAL_SET(cpusignal_console_halt, 0);
	/*
	 SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_pdp11.PC));
	 SIGNAL_SET(signals_cpu_generic.bus_data_register, SIGNAL_GET(signals_cpu_pdp11.instruction_register));
	 //SIGNAL_SET(signals_cpu_pdp11.bus_register, SIGNAL_GET(signals_cpu_pdp11.instruction_register));
	 _this->led_MASTER->value = 0; // sure ?
	 _this->led_PAUSE->value = 1; // see 1.3.4
	 _this->run_state = RUN_STATE_WAIT; // RUN led off
	 */
}

void realcons_console_pdp10__event_run_start(realcons_console_logic_pdp10_t *_this)
{
	if (_this->realcons->debug)
		printf("realcons_console_pdp10__event_run_start\n");
	/*
	 if (_this->switch_HALT->value)
	 return; // do not accept RUN command
	 // set property RUNMODE, so SimH can read it back
	 SIGNAL_SET(signals_cpu_generic.console_halt, 0); // running
	 _this->led_MASTER->value = 1; // sure ?
	 _this->led_PAUSE->value = 0; // see 1.3.4
	 */
	_this->run_state = RUN_STATE_RUN;
}

void realcons_console_pdp10__event_step_start(realcons_console_logic_pdp10_t *_this)
{
	if (_this->realcons->debug)
		printf("realcons_console_pdp10__event_step_start\n");
	SIGNAL_SET(cpusignal_console_halt, 0); // running
	/*
	 _this->led_MASTER->value = 1; // processor fetches, is unibus master
	 _this->led_PAUSE->value = 0; // see 1.3.4
	 */
}

void realcons_console_pdp10__event_operator_halt(realcons_console_logic_pdp10_t *_this)
{
	if (_this->realcons->debug)
		printf("realcons_console_pdp10__event_operator_halt\n");
	//STOP button is "momentary action". Remove Stop condition after scp acknowledges it
	SIGNAL_SET(cpusignal_console_halt, 0);
	/*
	 SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_pdp11.PC));
	 // SIGNAL_SET(signals_cpu_pdp11.ALU_result, SIGNAL_GET(signals_cpu_pdp11.PSW));
	 _this->led_MASTER->value = 1; // processor fetches, is unibus master
	 _this->led_PAUSE->value = 0; // see 1.3.4
	 */
	_this->run_state = RUN_STATE_HALT_MAN;

	// stop caused by RESET button? do reset
	if (_this->button_RESET.pendingbuttons) {
		sprintf(_this->realcons->simh_cmd_buffer, "reset all\n");
		_this->button_RESET.pendingbuttons = 0;
	}

}

void realcons_console_pdp10__event_step_halt(realcons_console_logic_pdp10_t *_this)
{
	if (_this->realcons->debug)
		printf("realcons_console_pdp10__event_step_halt\n");
	SIGNAL_SET(cpusignal_console_halt, 1);
	/*
	 SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_pdp11.PC));
	 // SIGNAL_SET(signals_cpu_pdp11.ALU_result, SIGNAL_GET(signals_cpu_pdp11.PSW));
	 _this->led_MASTER->value = 0; // sure ?
	 _this->led_PAUSE->value = 0; // see 1.3.4
	 */
	_this->run_state = RUN_STATE_HALT_MAN;
}

// exam and deposit
// addr and value in  signals_cpu_generic..bus_address_register / .bus_data_register
void realcons_console_pdp10__event_operator_exam_deposit(realcons_console_logic_pdp10_t *_this)
{
	if (_this->realcons->debug)
		printf("realcons_console_pdp10__event_operator_exam\n");

	// set data LEDs to value. data buttons unchanged
	// set memory indicator.
	// no address LEDS: button light instead!
	_this->memory_indicator_program = 0;
	realcons_pdp10_control_set(&_this->buttons_ADDRESS,
		SIGNAL_GET(cpusignal_memory_address_phys_register));
	realcons_pdp10_control_set(&_this->leds_DATA,
		SIGNAL_GET(cpusignal_memory_data_register));
	realcons_pdp10_control_set(&_this->led_MEMORY_DATA, 1);
	realcons_pdp10_control_set(&_this->led_PROGRAM_DATA, 0);
}

// if the program writes to console data or address registers,
// the MI indicator changes from  "MEMORY DATA" to "PROGRAM DATA"
// The new data value may not used with DATA LOAD or ADDR LOAD,
// until MI PROG DISABALE is set
// (to prevent fast changing values copied into the DATA/ADDR buttons ?)
// See bitsavers, pdf/dec/pdp10/TOPS10/1973_Assembly_Language_Handbook/01_1973AsmRef_SysRef.pdf
// document page "102".
void realcons_console_pdp10__event_program_write_memory_indicator(
	realcons_console_logic_pdp10_t *_this)
{
	// switch displaymode, if not disabled
	if (!realcons_pdp10_control_get(&_this->button_MI_PROG_DIS)) {
		_this->memory_indicator_program = 1; // MI PROG

		// the small triangle memory indicators are set in service()
// and update DATA (row4) and ADDR from CPU register, access udner program control)
		// DATA
		realcons_pdp10_control_set(&_this->leds_DATA,
			SIGNAL_GET(cpusignal_console_memory_indicator));
	}
}

// program has written to address and address condition switches
void realcons_console_pdp10__event_program_write_address_addrcond(
	realcons_console_logic_pdp10_t *_this)
{
	// switch displaymode, if not disabled
	if (!realcons_pdp10_control_get(&_this->button_MI_PROG_DIS)) {
		uint64_t address_addrcond = SIGNAL_GET(cpusignal_console_address_addrcond);
		// _this->memory_indicator_program = 1 ; // MI PROG? not, if address is written?

		// console_address_addrcond codes the ADDRESS SWITCHES
		// and INT FETCH, DATA FETCH, WRITE, ADDRESS BREAK EXEC PAGING USER, pAGINMHG
		// See bitsavers, pdf/dec/pdp10/TOPS10/1973_Assembly_Language_Handbook/01_1973AsmRef_SysRef.pdf
		// document page "103".

		// INST_FETCH = Bit 0(pdp10)
		realcons_pdp10_control_set(&_this->button_FETCH_INST,
			!!(address_addrcond & ((uint64_t)1 << (35 - 0))));
		// DATA_FETCH = Bit 1(pdp10)
		realcons_pdp10_control_set(&_this->button_FETCH_DATA,
			!!(address_addrcond & ((uint64_t)1 << (35 - 1))));
		// WRITE = Bit 4(pdp10)
		realcons_pdp10_control_set(&_this->button_WRITE,
			!!(address_addrcond & ((uint64_t)1 << (35 - 4))));
		// EXEC PAGING = Bit 5(pdp10)
		realcons_pdp10_control_set(&_this->button_PAGING_EXEC,
			!!(address_addrcond & ((uint64_t)1 << (35 - 5))));
		// USER PAGING = Bit 6(pdp10)
		realcons_pdp10_control_set(&_this->button_PAGING_USER,
			!!(address_addrcond & ((uint64_t)1 << (35 - 6))));
		// ADDRESS SWITCHES = Bits 7..35(pdp10) = lower 22 bits
		realcons_pdp10_control_set(&_this->buttons_ADDRESS, address_addrcond & 0x3fffffL);
	}
}


/* There was a deposit over simh console.
 * Convert to an event, if necessary.
 * reg is any simh register, not just one of ours.
 * Here events for access to the output registers "cim" and "caacs" are generated.
 * */
void realcons_console_pdp10_event_simh_deposit(realcons_console_logic_pdp10_t *_this,
struct REG *reg)
{
	// user made a "deposit CMI xxxx": flip "program data" ON
	if (reg->loc == _this->cpusignal_console_memory_indicator)  // cmi
		realcons_console_pdp10__event_program_write_memory_indicator(_this);
	if (reg->loc == _this->cpusignal_console_address_addrcond) // caacs
		realcons_console_pdp10__event_program_write_address_addrcond(_this);
}


void realcons_console_pdp10_interface_connect(realcons_console_logic_pdp10_t *_this,
	realcons_console_controller_interface_t *intf, char *panel_name)
{
	intf->name = panel_name;
	intf->constructor_func =
		(console_controller_constructor_func_t)realcons_console_pdp10_constructor;
	intf->destructor_func =
		(console_controller_destructor_func_t)realcons_console_pdp10_destructor;
	intf->reset_func = (console_controller_reset_func_t)realcons_console_pdp10_reset;
	intf->service_func = (console_controller_service_func_t)realcons_console_pdp10_service;
	intf->test_func = (console_controller_test_func_t)realcons_console_pdp10_test;

	intf->event_connect = (console_controller_event_func_t)realcons_console_pdp10_event_connect;
	intf->event_disconnect = (console_controller_event_func_t)realcons_console_pdp10_event_disconnect;

	// connect pdp11 cpu signals end events to simulator and realcons state variables
	{
		// REALCONS extension in scp.c
		extern t_addr realcons_memory_address_phys_register; // REALCONS extension in scp.c
         	extern char *realcons_register_name; // pseudo: name of last accessed register
		extern t_value realcons_memory_data_register; // REALCONS extension in scp.c
		extern  int realcons_console_halt; // 1: CPU halted by realcons console
		extern volatile t_bool sim_is_running; // global in scp.c
		// from pdp10_cpu.c
		extern int32 pager_PC; // programm counter ... pager OK?
	//	extern int32 SR, DR; // switch/display register, global in pdp11_cpu_mod.c
	//	extern int32 cm; // cpu mode, global in pdp11_cpu.c. MD:KRN_MD, MD_SUP,MD_USR,MD_UND
		extern int32 pi_enb, pi_act, pi_on, pi_prq, pi_ioq;
		extern uint64_t cds, cmi, caacs;
		extern	int32			realcons_PC; // own buffer!
		extern uint64_t		realcons_instruction; // current instr

		realcons_console_halt = 0;

		// from scp.c
		_this->cpusignal_memory_address_phys_register = &realcons_memory_address_phys_register;
                _this->cpusignal_register_name = &realcons_register_name; // pseudo: name of last accessed register
		_this->cpusignal_memory_data_register = &realcons_memory_data_register;
		_this->cpusignal_console_halt = &realcons_console_halt;
		// is "sim_is_running" indeed identical with our "cpu_is_running" ?
		// may cpu stops, but some device are still serviced?
		_this->cpusignal_run = &(sim_is_running);
		// from pdp10_cpu.c
		_this->cpusignal_PC = &realcons_PC; // own buffer!
		_this->cpusignal_instruction = &realcons_instruction; // current instruction
		_this->cpusignal_console_data_switches = &cds;
		_this->cpusignal_console_memory_indicator = &cmi;
		_this->cpusignal_console_address_addrcond = &caacs;
		_this->cpusignal_pi_on = &pi_on;
		_this->cpusignal_pi_enb = &pi_enb; // simh pdp10 cpu reg "PIENB"
		_this->cpusignal_pi_act = &pi_act; // simh	pdp10 cpu reg "PIACT"
		_this->cpusignal_pi_prq = &pi_prq; // simh	pdp10 cpu reg "PIPRQ"
		_this->cpusignal_pi_ioq = &pi_ioq; // simh	pdp10 cpu reg "PIIOQ"
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
		// pdp10_cpu.c
		extern console_controller_event_func_t realcons_event_opcode_any; // triggered after any opcode execution
		extern console_controller_event_func_t realcons_event_opcode_halt;
		extern console_controller_event_func_t realcons_event_opcode_reset; // triggered after execution of RESET
		extern console_controller_event_func_t realcons_event_opcode_wait; // triggered after execution of WAIT
		extern console_controller_event_func_t	realcons_event_program_write_memory_indicator;
		extern console_controller_event_func_t	realcons_event_program_write_address_addrcond;

		realcons_event_run_start =
			(console_controller_event_func_t)realcons_console_pdp10__event_run_start;
		realcons_event_step_start =
			(console_controller_event_func_t)realcons_console_pdp10__event_step_start;
		realcons_event_operator_halt =
			(console_controller_event_func_t)realcons_console_pdp10__event_operator_halt;
		realcons_event_step_halt =
			(console_controller_event_func_t)realcons_console_pdp10__event_step_halt;
		//exam, deposit handled by same routine
		realcons_event_operator_exam =
			(console_controller_event_func_t)realcons_console_pdp10__event_operator_exam_deposit;
		realcons_event_operator_deposit =
			(console_controller_event_func_t)realcons_console_pdp10__event_operator_exam_deposit;
        	realcons_event_operator_reg_exam = realcons_event_operator_reg_deposit = NULL ;
			realcons_event_cpu_reset = NULL ;
		realcons_event_opcode_any =
			(console_controller_event_func_t)realcons_console_pdp10__event_opcode_any;
		realcons_event_opcode_halt =
			(console_controller_event_func_t)realcons_console_pdp10__event_opcode_halt;
		realcons_event_opcode_reset =
			(console_controller_event_func_t)realcons_console_pdp10__event_opcode_reset;
		realcons_event_opcode_wait =
			(console_controller_event_func_t)realcons_console_pdp10__event_opcode_wait;
		realcons_event_program_write_memory_indicator =
			(console_controller_event_func_t)realcons_console_pdp10__event_program_write_memory_indicator;
		realcons_event_program_write_address_addrcond =
			(console_controller_event_func_t)realcons_console_pdp10__event_program_write_address_addrcond;
	}
}

// setup first state, KI10
t_stat realcons_console_pdp10_reset(realcons_console_logic_pdp10_t *_this)
{
	_this->realcons->simh_cmd_buffer[0] = '\0';

	_this->memory_indicator_program = 0;

	/*
	 * direct links to all required controls.
	 * Also check of config file
	 * Order is important!
	 * Action button priority is from left to right!
	 */
	realcons_pdp10_controls_count = 0; // empty list of controls

	// ---------- upper panel --------------------------

	// From doc: " the console data lock disables the data and sense switches;
	//	the console lock disables all other buttons except those that are mechanical"

	if (realcons_pdp10_control_init(&_this->lamp_OVERTEMP, _this->realcons,
		NULL, "OVERTEMP",
		REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->lamp_CKT_BRKR_TRIPPED, _this->realcons,
		NULL, "CKT_BRKR_TRIPPED",
		REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->lamp_DOORS_OPEN, _this->realcons,
		NULL, "DOORS_OPEN",
		REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;

	if (realcons_pdp10_control_init(&_this->knob_MARGIN_SELECT, _this->realcons, "MARGIN_SELECT",
		NULL,
		REALCONS_PDP10_CONTROL_MODE_INPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->knob_IND_SELECT, _this->realcons, "IND_SELECT",
		NULL,
		REALCONS_PDP10_CONTROL_MODE_INPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->knob_SPEED_CONTROL_COARSE, _this->realcons,
		"SPEED_CONTROL_COARSE", NULL,
		REALCONS_PDP10_CONTROL_MODE_INPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->knob_SPEED_CONTROL_FINE, _this->realcons,
		"SPEED_CONTROL_FINE", NULL,
		REALCONS_PDP10_CONTROL_MODE_INPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->knob_MARGIN_VOLTAGE, _this->realcons, "MARGIN_VOLTAGE",
		NULL,
		REALCONS_PDP10_CONTROL_MODE_INPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->VOLTMETER, _this->realcons,
		NULL, "VOLTMETER",
		REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->enable_HOURMETER, _this->realcons,
		NULL, "HOURMETER",
		REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;

	if (realcons_pdp10_control_init(&_this->button_FM_MANUAL, _this->realcons, "FM_MANUAL_SW",
		"FM_MANUAL_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->buttons_FM_BLOCK, _this->realcons, "FM_BLOCK_SW",
		"FM_BLOCK_FB",
		REALCONS_PDP10_CONTROL_MODE_SWITCH) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->buttons_SENSE, _this->realcons, "SENSE_SW", "SENSE_FB",
		REALCONS_PDP10_CONTROL_MODE_SWITCH) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_MI_PROG_DIS, _this->realcons, "MI_PROG_DIS_SW",
		"MI_PROG_DIS_FB",
		REALCONS_PDP10_CONTROL_MODE_SWITCH) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_MEM_OVERLAP_DIS, _this->realcons,
		"MEM_OVERLAP_DIS_SW", "MEM_OVERLAP_DIS_FB",
		REALCONS_PDP10_CONTROL_MODE_SWITCH) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_SINGLE_PULSE, _this->realcons, "SINGLE_PULSE_SW",
		"SINGLE_PULSE_FB",
		REALCONS_PDP10_CONTROL_MODE_SWITCH) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_MARGIN_ENABLE, _this->realcons,
		"MARGIN_ENABLE_SW", "MARGIN_ENABLE_FB",
		REALCONS_PDP10_CONTROL_MODE_SWITCH) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->buttons_MANUAL_MARGIN_ADDRESS, _this->realcons,
		"MANUAL_MARGIN_ADDRESS_SW", "MANUAL_MARGIN_ADDRESS_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	// toggle, aber selber rastend!
	if (realcons_pdp10_control_init(&_this->buttons_READ_IN_DEVICE, _this->realcons,
		"READ_IN_DEVICE_SW", "READ_IN_DEVICE_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;

	if (realcons_pdp10_control_init(&_this->button_LAMP_TEST, _this->realcons, "LAMP_TEST_SW", NULL,
		REALCONS_PDP10_CONTROL_MODE_INPUT) != SCPE_OK)
		return SCPE_NOATT;
	// nuild in lamp feedback
	if (realcons_pdp10_control_init(&_this->button_CONSOLE_LOCK, _this->realcons, "CONSOLE_LOCK_SW",
		NULL,
		REALCONS_PDP10_CONTROL_MODE_INPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_CONSOLE_DATALOCK, _this->realcons,
		"CONSOLE_DATALOCK_SW", NULL,
		REALCONS_PDP10_CONTROL_MODE_INPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_POWER, _this->realcons,
		"POWERBUTTON_SW", NULL,
		REALCONS_PDP10_CONTROL_MODE_INPUT) != SCPE_OK)
		return SCPE_NOATT;

	// ---------- LEDs on lower panel --------------------------
	if (realcons_pdp10_control_init(&_this->leds_PI_ACTIVE, _this->realcons,
		NULL, "PI_ACTIVE", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->leds_IOB_PI_REQUEST, _this->realcons,
		NULL, "IOB_PI_REQUEST", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->leds_PI_IN_PROGRESS, _this->realcons,
		NULL, "PI_IN_PROGRESS", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->leds_PI_REQUEST, _this->realcons,
		NULL, "PI_REQUEST", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->led_PI_ON, _this->realcons,
		NULL, "PI_ON", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->led_PI_OK_8, _this->realcons,
		NULL, "PI_OK_8", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;

	// 0x01 = EXEC_MODE_KERNEL, 0x02 = EXEC_MODE_SUPER, 0x04 = USER_MODE_CONCEAL, 0x08 = USER_MODE_PUBLIC
	if (realcons_pdp10_control_init(&_this->leds_MODE, _this->realcons,
		NULL, "MODE", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->led_KEY_PG_FAIL, _this->realcons,
		NULL, "KEY_PG_FAIL", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->led_KEY_MAINT, _this->realcons,
		NULL, "KEY_MAINT", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	// 0x01 = STOP_MEM, 0x02 = STOP_PROG, 0x04 = STOP_MAN
	if (realcons_pdp10_control_init(&_this->leds_STOP, _this->realcons,
		NULL, "STOP", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->led_RUN, _this->realcons,
		NULL, "RUN", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->led_POWER, _this->realcons,
		NULL, "POWER", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;

	if (realcons_pdp10_control_init(&_this->leds_PROGRAM_COUNTER, _this->realcons,
		NULL, "PROGRAM_COUNTER", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->leds_INSTRUCTION, _this->realcons,
		NULL, "INSTRUCTION", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->led_PROGRAM_DATA, _this->realcons,
		NULL, "PROGRAM_DATA", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->led_MEMORY_DATA, _this->realcons,
		NULL, "MEMORY_DATA", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->leds_DATA, _this->realcons,
		NULL, "DATA", REALCONS_PDP10_CONTROL_MODE_OUTPUT) != SCPE_OK)
		return SCPE_NOATT;

	// ----- buttons on lower panel --------------
	// "In the upper half of the operator panel are four rows of indicators, and below them are three
	// rows of two-position keys and switches. Physically both are pushbuttons, but the keys are
	// momentary contact whereas the switches are alternate action.""
	if (realcons_pdp10_control_init(&_this->button_PAGING_EXEC, _this->realcons, "PAGING_EXEC_SW",
		"PAGING_EXEC_FB",
		REALCONS_PDP10_CONTROL_MODE_SWITCH) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_PAGING_USER, _this->realcons, "PAGING_USER_SW",
		"PAGING_USER_FB",
		REALCONS_PDP10_CONTROL_MODE_SWITCH) != SCPE_OK)
		return SCPE_NOATT;

	if (realcons_pdp10_control_init(&_this->buttons_ADDRESS, _this->realcons, "ADDRESS_SW",
		"ADDRESS_FB", REALCONS_PDP10_CONTROL_MODE_SWITCH) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_ADDRESS_CLEAR, _this->realcons,
		"ADDRESS_CLEAR_SW", "ADDRESS_CLEAR_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_ADDRESS_LOAD, _this->realcons, "ADDRESS_LOAD_SW",
		"ADDRESS_LOAD_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;

	if (realcons_pdp10_control_init(&_this->buttons_DATA, _this->realcons, "DATA_SW", "DATA_FB",
		REALCONS_PDP10_CONTROL_MODE_SWITCH) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_DATA_CLEAR, _this->realcons, "DATA_CLEAR_SW",
		"DATA_CLEAR_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_DATA_LOAD, _this->realcons, "DATA_LOAD_SW",
		"DATA_LOAD_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;

	if (realcons_pdp10_control_init(&_this->button_SINGLE_INST, _this->realcons, "SINGLE_INST_SW",
		"SINGLE_INST_FB",
		REALCONS_PDP10_CONTROL_MODE_SWITCH) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_SINGLE_PULSER, _this->realcons,
		"SINGLE_PULSER_SW", "SINGLE_PULSER_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_STOP_PAR, _this->realcons, "STOP_PAR_SW",
		"STOP_PAR_FB", REALCONS_PDP10_CONTROL_MODE_SWITCH) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_STOP_NXM, _this->realcons, "STOP_NXM_SW",
		"STOP_NXM_FB", REALCONS_PDP10_CONTROL_MODE_SWITCH) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_REPEAT, _this->realcons, "REPEAT_SW",
		"REPEAT_FB", REALCONS_PDP10_CONTROL_MODE_SWITCH) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_FETCH_INST, _this->realcons, "FETCH_INST_SW",
		"FETCH_INST_FB", REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_FETCH_DATA, _this->realcons, "FETCH_DATA_SW",
		"FETCH_DATA_FB", REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_WRITE, _this->realcons, "WRITE_SW", "WRITE_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_ADDRESS_STOP, _this->realcons, "ADDRESS_STOP_SW",
		"ADDRESS_STOP_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_ADDRESS_BREAK, _this->realcons,
		"ADDRESS_BREAK_SW", "ADDRESS_BREAK_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_READ_IN, _this->realcons, "READ_IN_SW",
		"READ_IN_FB", REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_START, _this->realcons, "START_SW", "START_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_CONT, _this->realcons, "CONT_SW", "CONT_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_STOP, _this->realcons, "STOP_SW", "STOP_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_RESET, _this->realcons, "RESET_SW", "RESET_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_XCT, _this->realcons, "XCT_SW", "XCT_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;

	if (realcons_pdp10_control_init(&_this->button_EXAMINE_THIS, _this->realcons, "EXAMINE_THIS_SW",
		"EXAMINE_THIS_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_EXAMINE_NEXT, _this->realcons, "EXAMINE_NEXT_SW",
		"EXAMINE_NEXT_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_DEPOSIT_THIS, _this->realcons, "DEPOSIT_THIS_SW",
		"DEPOSIT_THIS_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;
	if (realcons_pdp10_control_init(&_this->button_DEPOSIT_NEXT, _this->realcons, "DEPOSIT_NEXT_SW",
		"DEPOSIT_NEXT_FB",
		REALCONS_PDP10_CONTROL_MODE_KEY) != SCPE_OK)
		return SCPE_NOATT;

	return SCPE_OK;
}

// process panel state.
// operates on Blinkenlight_API panel structs,
// but RPC communication is done external
// all input controls are read from panel before call
// all output controls are written back to panel after call
t_stat realcons_console_pdp10_service(realcons_console_logic_pdp10_t *_this)
{
	unsigned i;
	blinkenlight_panel_t *p = _this->realcons->console_model; // alias

	// call the service function for all controls
	for (i = 0; realcons_pdp10_controls[i]; i++) {
		realcons_pdp10_control_service(realcons_pdp10_controls[i]);
	}
	//	if (_this->buttons_READ_IN_DEVICE.buttons->value == 0)
	//		return SCPE_OK; //error stop

	// big service routine is broken up ...
	realcons_console_pdp10_maintpanel_service(_this);

	if (_this->realcons->lamp_test)
		return SCPE_OK; // no more light logic needed

	realcons_pdp10_operpanel_service(_this);

	return SCPE_OK;

}

/*
 * start 1 sec test sequence.
 * - lamps ON for 1 sec
 * - print state of all switches
 */
int realcons_console_pdp10_test(realcons_console_logic_pdp10_t *_this, int arg)
{
	unsigned i;
	realcons_pdp10_control_t *c;

	// send end time for test: 1 second = curtime + 1000
	// lamp test is set in service()
	_this->realcons->timer_running_msec[TIMER_TEST] = _this->realcons->service_cur_time_msec
		+ TIME_TEST_MS;

	// lamps are set ON in _service() by monitoring the timer
	realcons_printf(_this->realcons, stdout, "Verify lamp test!\n");

	// print state of all buttons
	for (i = 0; (c = realcons_pdp10_controls[i]); i++)
		if (c->buttons) {
			realcons_printf(_this->realcons, stdout, "Switch %-20s = %llo\n", c->buttons->name,
				c->buttons->value);
		}

	return 0; // OK

}
