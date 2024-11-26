/* realcons_pdp11_40.c: Logic for the specific 11/40 panel: KY11-D

   Copyright (c) 2012-2018, Joerg Hoppe
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

   21-Mar-2018	JH		Reset (HALT+START) load PC, merged with current SimH
   18-Mar-2016	JH		access to SimHs I/O page and CPU registers (18->22bit addr)
   20-Feb-2016  JH      added PANEL_MODE_POWERLESS
      Dec-2015  JH      migration from SimH 3.82 to 4.x
                        CPU extensions moved to pdp11_cpu.c
   25-Mar-2012  JH      created


   Inputs:
   - realcons_machine state (PDP11 as set by SimH)
   - blinkenlight API panel input (switches)

   Outputs:
   - realcons_machine state (as checked by SimH, example: HALT state)
   - blinkenlight API panel outputs (lamps)
   - SimH command string

   Class hierarchy:
   base panel = generic machine with properties requiered by SimH
  		everthing accessed in scp.c is "base"

   inherited pdp11_40 (or other architecure)
     an actual Panel has an actual machine state.

   (For instance, a 11/05 has almost the same machine state,
   but quite a different panel)

 */
#ifdef USE_REALCONS

#include <assert.h>

#include "sim_defs.h"
#include "realcons.h"
#include "realcons_console_pdp11_40.h"

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
 * translate console adresses to SimH 22 bit addr / registers
 * SimH specifies all adresses 22bit, even for 16 and 18 bit machines
 * so panel I/O page addresses 760000-777777 must translated to
 *	17760000-17777777
 * The 11/40 does not "inc by one" on CPU registers, is implemented here anyhow
 * the address LSB is set to 0.
 *
 * Also SimH gives not the CPU registers, if their IO page addresses are exam/deposit
 * So convert addr -> reg, and revers in the *_operator_reg_exam_deposit()
 */
static t_addr realcons_console_pdp11_40_addr_inc(t_addr addr) {
		if (addr >= 0777700 && addr <= 0777707)
			addr += 1; // inc to next register
		else addr += 2; // inc by word
		addr &= 0777777; // trunc to 18 bit
		return addr;
}

static char *realcons_console_pdp11_40_addr_panel2simh(t_addr panel_addr)
{
	static char buff[40];
	switch (panel_addr) {
	case 0777700: return "r0";
	case 0777701: return "r1";
	case 0777702: return "r2";
	case 0777703: return "r3";
	case 0777704: return "r4";
	case 0777705: return "r5";
	case 0777706: return "sp";
	case 0777707: return "pc";
	default:
		panel_addr &= ~1; // clear LSB, according to DEC doc
		if (panel_addr >= 0760000)
			panel_addr += 017000000; // 18->22 bit I/O page
		sprintf(buff, "%o", panel_addr);
		return buff;
	}
}

// only registers encode before must be back converted to addresses
static t_addr realcons_console_pdp11_40_addr_reg(char	*regname)
{
	if (!strcasecmp(regname, "r0")) return 0777700 ;
	if (!strcasecmp(regname, "r1")) return 0777701 ;
	if (!strcasecmp(regname, "r2")) return 0777702 ;
	if (!strcasecmp(regname, "r3")) return 0777703 ;
	if (!strcasecmp(regname, "r4")) return 0777704 ;
	if (!strcasecmp(regname, "r5")) return 0777705 ;
	if (!strcasecmp(regname, "sp")) return 0777706 ;
	if (!strcasecmp(regname, "pc")) return 0777707 ;
	fprintf(stderr, "REALCONS progrm error: unknown register in ...addr_reg()\n");
	exit(1);
}



/*
 * constructor / destructor
 */
realcons_console_logic_pdp11_40_t *realcons_console_pdp11_40_constructor(realcons_t *realcons)
{
	realcons_console_logic_pdp11_40_t *_this;

	_this = (realcons_console_logic_pdp11_40_t *)malloc(sizeof(realcons_console_logic_pdp11_40_t));
	_this->realcons = realcons; // connect to parent
	_this->run_state = 0;

	return _this;
}

void realcons_console_pdp11_40_destructor(realcons_console_logic_pdp11_40_t *_this)
{
	free(_this);
}

/*
 * Interface to external simulation:
 * signaled by SimH for state changes of CPU
 * Here used to set DMUX, which is used to drive DATA leds
 *
 * CPU conditions:
 * HALT instruction has R(00)
 * RESET instruction has R(00)
 * WAIT instruction has R(IR)
 * SINGLE STEP and HALT switch has Processor Status (PS)
 *
 * Console switches:
 * LOAD ADRS - the transferred Switch register address.
 * DEP - the Switch register data just deposited.
 * EXAM - the information from the address examined.
 * HALT - displays the current Processor Status (PS) word.
 *
 * Called high speed in simulator loop!
 * return 0, if not accepted
 */

 // SHORTER macros for signal access
 // assume "realcons_console_logic_pdp11_40_t *_this" in context
#define SIGNAL_SET(signal,value) REALCONS_SIGNAL_SET(_this,signal,value)
#define SIGNAL_GET(signal) REALCONS_SIGNAL_GET(_this,signal)

void realcons_console_pdp11_40_event_connect(realcons_console_logic_pdp11_40_t *_this)
{
   // set panel mode to "powerless". all lights go off,
   // On Java panels the power switch should flip to the ON position
	realcons_power_mode(_this->realcons, 1);
}

void realcons_console_pdp11_40_event_disconnect(realcons_console_logic_pdp11_40_t *_this)
{
	// set panel mode to "powerless". all lights go off,
	// On Java panels the power switch should flip to the OFF position
	realcons_power_mode(_this->realcons, 0);
}

void realcons_console_pdp11_40__event_opcode_any(realcons_console_logic_pdp11_40_t *_this)
{
	// other opcodes executed by processor
	if (_this->realcons->debug)
		printf("realcons_console_pdp11_40__event_opcode_any\n");
	// after any opcode: ADDR shows PC, DATA shows IR = opcode
	SIGNAL_SET(cpusignal_memory_address_phys_register, SIGNAL_GET(cpusignal_PC));
	_this->DMUX = SIGNAL_GET(cpusignal_instruction_register);
	// show opcode
	_this->led_BUS->value = 1; // UNIBUS cycles ....
	_this->led_PROCESSOR->value = 1; // ... by processor
	_this->run_state = RUN_STATE_RUN;
}

void realcons_console_pdp11_40__event_opcode_halt(realcons_console_logic_pdp11_40_t *_this)
{
	if (_this->realcons->debug)
		printf("realcons_console_pdp11_40__event_opcode_halt\n");
	SIGNAL_SET(cpusignal_memory_address_phys_register, SIGNAL_GET(cpusignal_PC));
	_this->DMUX = SIGNAL_GET(cpusignal_R0);
	_this->led_BUS->value = 0; // no UNIBUS cycles any more
	_this->led_PROCESSOR->value = 0; // processor stop
	_this->run_state = RUN_STATE_HALT;
}

void realcons_console_pdp11_40__event_opcode_reset(realcons_console_logic_pdp11_40_t *_this)
{
	if (_this->realcons->debug)
		printf("realcons_console_pdp11_40__event_opcode_reset\n");
	SIGNAL_SET(cpusignal_memory_address_phys_register, SIGNAL_GET(cpusignal_PC));
	_this->DMUX = SIGNAL_GET(cpusignal_R0);
	_this->led_BUS->value = 1; // http://www.youtube.com/watch?v=iIsZVqhaneo
	_this->led_PROCESSOR->value = 1;
	_this->run_state = RUN_STATE_RESET;

    // the RESET opcode sets the UNIBUS signal INIT active for 20ms
    // After that there's a 50ms delay
    // PDP-11-40 system engineering drawings, Rev P(Jun 1974, Rev P),
    // page  "FLOW DIAGRAM (TRAPS,PWRUP,RESET,RTI,RTS,RTT) (6)"
    // Some programs use it as display delay to show R0
    realcons_ms_sleep(_this->realcons, 70); // keep realcons logic active

}

void realcons_console_pdp11_40__event_opcode_wait(realcons_console_logic_pdp11_40_t *_this)
{
	if (_this->realcons->debug)
		printf("realcons_console_pdp11_40__event_opcode_wait\n");
	SIGNAL_SET(cpusignal_memory_address_phys_register, SIGNAL_GET(cpusignal_PC));
	_this->DMUX = SIGNAL_GET(cpusignal_instruction_register);
	_this->led_BUS->value = 0; // no UNIBUS cycles any more
	_this->led_PROCESSOR->value = 0; // processor idle
	_this->run_state = RUN_STATE_WAIT; // RUN led off
}

void realcons_console_pdp11_40__event_run_start(realcons_console_logic_pdp11_40_t *_this)
{
	if (_this->realcons->debug)
		printf("realcons_console_pdp11_40__event_run_start\n");
	if (_this->switch_HALT->value)
		return; // do not accept RUN command
	// set property RUNMODE, so SimH can read it back
	_this->led_BUS->value = 1; // start accessing UNIBUS cycles
	_this->led_PROCESSOR->value = 1; // processor active
	_this->run_state = RUN_STATE_RUN;
}

void realcons_console_pdp11_40__event_step_start(realcons_console_logic_pdp11_40_t *_this)
{
	if (_this->realcons->debug)
		printf("realcons_console_pdp11_40__event_step_start\n");
	_this->led_BUS->value = 1; // start accessing UNIBUS cycles
	_this->led_PROCESSOR->value = 1; // processor active
}

// after single step and on operator halt the same happens
void realcons_console_pdp11_40__event_operator_step_halt(realcons_console_logic_pdp11_40_t *_this)
{
	if (_this->realcons->debug)
		printf("realcons_console_pdp11_40__event_operator_step_halt\n");
	SIGNAL_SET(cpusignal_memory_address_phys_register, SIGNAL_GET(cpusignal_PC));
	_this->DMUX = SIGNAL_GET(cpusignal_PSW);
	_this->led_BUS->value = 0; // no UNIBUS cycles any more
	_this->led_PROCESSOR->value = 0; // processor idle
	_this->run_state = RUN_STATE_HALT;
}
void realcons_console_pdp11_40__event_operator_exam_deposit(realcons_console_logic_pdp11_40_t *_this)
{
	if (_this->realcons->debug)
		printf("realcons_console_pdp11_40__event_operator_exam_deposit\n");
	// exam on SimH console sets also console address (like LOAD ADR)
	_this->console_address_register = SIGNAL_GET(cpusignal_memory_address_phys_register);
	_this->DMUX = SIGNAL_GET(cpusignal_memory_data_register);
	_this->led_BUS->value = 0; // no UNIBUS cycles any more
	_this->led_PROCESSOR->value = 0; // processor idle
}

// Called if Realcons or a user typed "examine <regname>" or "deposit <regname> value".
// Realcons accesses only the CPU registers by name, so only these are recognized here
// see realcons_console_pdp11_40_addr_panel2simh() !
void realcons_console_pdp11_40__event_operator_reg_exam_deposit(realcons_console_logic_pdp11_40_t *_this)
{
	char *regname = SIGNAL_GET(cpusignal_register_name); // alias
	t_addr	addr = 0;
	if (_this->realcons->debug)
		printf("realcons_console_pdp11_70__event_operator_reg_exam_deposit\n");
	// exam on SimH console sets also console address (like LOAD ADR)
	// convert register into UNIBUS address
	if (!strcasecmp(regname, "r0")) addr = 0777700;
	else if (!strcasecmp(regname, "r1")) addr = 0777701;
	else if (!strcasecmp(regname, "r2")) addr = 0777702;
	else if (!strcasecmp(regname, "r3")) addr = 0777703;
	else if (!strcasecmp(regname, "r4")) addr = 0777704;
	else if (!strcasecmp(regname, "r5")) addr = 0777705;
	else if (!strcasecmp(regname, "sp")) addr = 0777706;
	else if (!strcasecmp(regname, "pc")) addr = 0777707;
	if (addr) {
		SIGNAL_SET(cpusignal_memory_address_phys_register, addr);
		realcons_console_pdp11_40__event_operator_exam_deposit(_this);
	} // other register access are ignored by the panel


}

// Called after CPU has been reinitialized
void realcons_console_pdp11_40__event_cpu_reset(
        realcons_console_logic_pdp11_40_t *_this) {
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_40__event_cpu_reset\n");

    // load PC, show PC in ADDR
    SIGNAL_SET(cpusignal_PC, _this->console_address_register) ;
	SIGNAL_SET(cpusignal_memory_address_phys_register, _this->console_address_register) ;
}



void realcons_console_pdp11_40_interface_connect(realcons_console_logic_pdp11_40_t *_this,
	realcons_console_controller_interface_t *intf, char *panel_name)
{
	intf->name = panel_name;
	intf->constructor_func =
		(console_controller_constructor_func_t)realcons_console_pdp11_40_constructor;
	intf->destructor_func =
		(console_controller_destructor_func_t)realcons_console_pdp11_40_destructor;
	intf->reset_func = (console_controller_reset_func_t)realcons_console_pdp11_40_reset;
	intf->service_func = (console_controller_service_func_t)realcons_console_pdp11_40_service;
	intf->test_func = (console_controller_test_func_t)realcons_console_pdp11_40_test;

	intf->event_connect = (console_controller_event_func_t)realcons_console_pdp11_40_event_connect;
	intf->event_disconnect = (console_controller_event_func_t)realcons_console_pdp11_40_event_disconnect;


	// connect pdp11 cpu signals end events to simulator and realcons state variables
	{
		// REALCONS extension in scp.c
		extern t_addr realcons_memory_address_phys_register; // REALCONS extension in scp.c
		extern char *realcons_register_name; // pseudo: name of last accessed register
		extern t_value realcons_memory_data_register; // REALCONS extension in scp.c
		extern  int realcons_console_halt; // 1: CPU halted by realcons console
		extern volatile t_bool sim_is_running; // global in scp.c

		extern int32 R[8]; // working registers, global in pdp11_cpu.c
        extern int32 saved_PC;
		extern int32 SR, DR; // switch/display register, global in pdp11_cpu_mod.c
		extern int32 cm; // cpu mode, global in pdp11_cpu.c. MD:KRN_MD, MD_SUP,MD_USR,MD_UND
		extern int 	realcons_bus_ID_mode; // 1 = DATA space access, 0 = instruction space access
		extern t_value realcons_DATAPATH_shifter; // output of ALU
		extern t_value realcons_IR; // buffer for instruction register (opcode)
		extern t_value realcons_PSW; // buffer for program status word

		realcons_console_halt = 0;

		// from scp.c
		_this->cpusignal_memory_address_phys_register = &realcons_memory_address_phys_register;
		_this->cpusignal_register_name = &realcons_register_name; // pseudo: name of last accessed register
		_this->cpusignal_memory_data_register = &realcons_memory_data_register;
		_this->cpusignal_console_halt = &realcons_console_halt;

		// from pdp11_cpu.c
	// is "sim_is_running" indeed identical with our "cpu_is_running" ?
	// may cpu stops, but some device are still serviced?
		_this->cpusignal_run = &(sim_is_running);

		_this->cpusignal_DATAPATH_shifter = &realcons_DATAPATH_shifter; // not used
		// 11/70 has a bus register BR: is this the bus data register???
		//_this->signals_cpu_pdp11.bus_register = &(cpu_state->bus_data_register);

		// set by LOAD ADDR, on all PDP11's
		// signal from realcons console to CPU: 1=HALTed
		// oder gleich "&(_switch_HALT->value)?"
		_this->cpusignal_instruction_register = &realcons_IR;
		_this->cpusignal_PSW = &realcons_PSW;
		_this->cpusignal_bus_ID_mode = &realcons_bus_ID_mode;
		_this->cpusignal_cpu_mode = (t_value*)&cm; // MD_SUP,MD_
		_this->cpusignal_R0 = (t_value*)&(R[0]); // R: global of pdp11_cpu.c
		_this->cpusignal_PC = &saved_PC ; // R[7] not valid in console mode
		_this->cpusignal_switch_register = (t_value*)&SR; // see pdp11_cpumod.SR_rd()
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
		// pdp11_cpu.c
		extern console_controller_event_func_t realcons_event_opcode_any; // triggered after any opcode execution
		extern console_controller_event_func_t realcons_event_opcode_halt;
		extern console_controller_event_func_t realcons_event_opcode_reset; // triggered after execution of RESET
		extern console_controller_event_func_t realcons_event_opcode_wait; // triggered after execution of WAIT

		realcons_event_run_start =
			(console_controller_event_func_t)realcons_console_pdp11_40__event_run_start;
		realcons_event_step_start =
			(console_controller_event_func_t)realcons_console_pdp11_40__event_step_start;
		realcons_event_operator_halt =
			realcons_event_step_halt =
			(console_controller_event_func_t)realcons_console_pdp11_40__event_operator_step_halt;
		realcons_event_operator_exam =
			realcons_event_operator_deposit =
			(console_controller_event_func_t)realcons_console_pdp11_40__event_operator_exam_deposit;
		realcons_event_operator_reg_exam =
			realcons_event_operator_reg_deposit =
			(console_controller_event_func_t)realcons_console_pdp11_40__event_operator_reg_exam_deposit;
		realcons_event_cpu_reset =
			(console_controller_event_func_t) realcons_console_pdp11_40__event_cpu_reset ;
		realcons_event_opcode_any =
			(console_controller_event_func_t)realcons_console_pdp11_40__event_opcode_any;
		realcons_event_opcode_halt =
			(console_controller_event_func_t)realcons_console_pdp11_40__event_opcode_halt;
		realcons_event_opcode_reset =
			(console_controller_event_func_t)realcons_console_pdp11_40__event_opcode_reset;
		realcons_event_opcode_wait =
			(console_controller_event_func_t)realcons_console_pdp11_40__event_opcode_wait;
	}

}

// setup first state
t_stat realcons_console_pdp11_40_reset(realcons_console_logic_pdp11_40_t *_this)
{
	_this->realcons->simh_cmd_buffer[0] = '\0';
    _this->console_address_register = 0;
	//_this->sig_R_ADRSC = 0 ;
	_this->autoinc_addr_action_switch = NULL; // not active

	_this->console_address_register = 0 ;

	/*
	 * direct links to all required controls.
	 * Also check of config file
	 */
	if (!(_this->switch_SR = realcons_console_get_input_control(_this->realcons, "SR")))
		return SCPE_NOATT;
	if (!(_this->switch_LOAD_ADRS = realcons_console_get_input_control(_this->realcons, "LOAD ADRS")))
		return SCPE_NOATT;
	if (!(_this->switch_EXAM = realcons_console_get_input_control(_this->realcons, "EXAM")))
		return SCPE_NOATT;
	if (!(_this->switch_CONT = realcons_console_get_input_control(_this->realcons, "CONT")))
		return SCPE_NOATT;
	if (!(_this->switch_HALT = realcons_console_get_input_control(_this->realcons, "HALT")))
		return SCPE_NOATT;
	if (!(_this->switch_START = realcons_console_get_input_control(_this->realcons, "START")))
		return SCPE_NOATT;
	if (!(_this->switch_DEPOSIT = realcons_console_get_input_control(_this->realcons, "DEPOSIT")))
		return SCPE_NOATT;

	if (!(_this->led_ADDRESS = realcons_console_get_output_control(_this->realcons, "ADDRESS")))
		return SCPE_NOATT;
	if (!(_this->led_DATA = realcons_console_get_output_control(_this->realcons, "DATA")))
		return SCPE_NOATT;
	if (!(_this->led_RUN = realcons_console_get_output_control(_this->realcons, "RUN")))
		return SCPE_NOATT;
	if (!(_this->led_BUS = realcons_console_get_output_control(_this->realcons, "BUS")))
		return SCPE_NOATT;
	if (!(_this->led_USER = realcons_console_get_output_control(_this->realcons, "USER")))
		return SCPE_NOATT;
	if (!(_this->led_PROCESSOR = realcons_console_get_output_control(_this->realcons, "PROCESSOR")))
		return SCPE_NOATT;
	if (!(_this->led_CONSOLE = realcons_console_get_output_control(_this->realcons, "CONSOLE")))
		return SCPE_NOATT;
	if (!(_this->led_VIRTUAL = realcons_console_get_output_control(_this->realcons, "VIRTUAL")))
		return SCPE_NOATT;
	return SCPE_OK;
}

// process panel state.
// operates on Blinkenlight_API panel structs,
// but RPC communication is done external
t_stat realcons_console_pdp11_40_service(realcons_console_logic_pdp11_40_t *_this)
{
	blinkenlight_panel_t *p = _this->realcons->console_model; // alias
	int console_mode;
	int user_mode;

	blinkenlight_control_t *action_switch; // current action switch

	/* test time expired? */

	// STOP by activating HALT?
	if (_this->switch_HALT->value && !SIGNAL_GET(cpusignal_console_halt)) {
		// must be done by SimH.pdp11_cpu.c!
	}

	// run mode KERNEL,SUPERVISOR, USER?
	user_mode = (SIGNAL_GET(cpusignal_cpu_mode) == REALCONS_CPU_PDP11_CPUMODE_USER);

	// CONSOLE: processor accepts commands from console panel when HALTed
	console_mode = !SIGNAL_GET(cpusignal_run)
		|| SIGNAL_GET(cpusignal_console_halt);

	/*************************************************************
	 * eval switches
	 * react on changed action switches
	 */

	 // fetch switch register
	SIGNAL_SET(cpusignal_switch_register, (t_value)_this->switch_SR->value);

	// fetch HALT mode, must be sensed by simulated processor to produce state OPERATOR_HALT
	SIGNAL_SET(cpusignal_console_halt, (t_value)_this->switch_HALT->value);

	/* which command switch was activated? Process only one of these */
	action_switch = NULL;
	if (!action_switch && _this->switch_LOAD_ADRS->value == 1
		&& _this->switch_LOAD_ADRS->value_previous == 0)
		action_switch = _this->switch_LOAD_ADRS;
	if (!action_switch && _this->switch_EXAM->value == 1 && _this->switch_EXAM->value_previous == 0)
		action_switch = _this->switch_EXAM;
	if (!action_switch && _this->switch_DEPOSIT->value == 1
		&& _this->switch_DEPOSIT->value_previous == 0)
		action_switch = _this->switch_DEPOSIT;
	if (!action_switch && _this->switch_CONT->value == 1 && _this->switch_CONT->value_previous == 0)
		action_switch = _this->switch_CONT;
	if (!action_switch && // START actions on rising and falling edge!
		_this->switch_START->value ^ _this->switch_START->value_previous)
		action_switch = _this->switch_START;

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

	 // accept input only in CONSOLE mode. Exception: HALT is always enabled, see above
	if (!console_mode)
		action_switch = NULL;

	if (action_switch) {
		/* auto addr inc logic */
		if (action_switch != _this->autoinc_addr_action_switch)
			// change of switch: DEP or EXAM sequence broken
			_this->autoinc_addr_action_switch = NULL;
		else
			// inc panel address register
			_this->console_address_register =
				realcons_console_pdp11_40_addr_inc(_this->console_address_register);

		if (action_switch == _this->switch_LOAD_ADRS) {
			_this->console_address_register =
				(realcons_machine_word_t)(_this->switch_SR->value & 0x3ffff); // 18 bit
			SIGNAL_SET(cpusignal_memory_address_phys_register,
				(realcons_machine_word_t)(_this->switch_SR->value & 0x3ffff)); // 18 bit
		// _this->DMUX = _this->R_ADRSC; // for display on DATA, DEC docs
			_this->DMUX = 0; // videos show: DATA is cleared
			// LOAD ADR active: copy switches to R_
			if (_this->realcons->debug)
				printf("LOADADR %o\n", _this->console_address_register);
			// flash with DATA LEDs
			_this->realcons->timer_running_msec[TIMER_DATA_FLASH] =
				_this->realcons->service_cur_time_msec + TIME_DATA_FLASH_MS;
		}

		if (action_switch == _this->switch_EXAM) {
			_this->autoinc_addr_action_switch = _this->switch_EXAM; // inc addr on next EXAM
			// generate simh "exam cmd"
			// fix octal, should use SimH-radix
			sprintf(_this->realcons->simh_cmd_buffer, "examine %s\n",
				realcons_console_pdp11_40_addr_panel2simh(
				_this->console_address_register));
		}

		if (action_switch == _this->switch_DEPOSIT) {
			unsigned dataval;
			dataval = (realcons_machine_word_t)_this->switch_SR->value & 0xffff; // trunc to switches 15..0
			_this->autoinc_addr_action_switch = _this->switch_DEPOSIT; // inc addr on next DEP

			// produce SimH cmd. fix octal, should use SimH-radix
			sprintf(_this->realcons->simh_cmd_buffer, "deposit %s %o\n",
				realcons_console_pdp11_40_addr_panel2simh(_this->console_address_register),
				dataval);
			// flash with DATA LEDs
			_this->realcons->timer_running_msec[TIMER_DATA_FLASH] =
				_this->realcons->service_cur_time_msec + TIME_DATA_FLASH_MS;
		}

		/* function of CONT, START mixed with HALT:
		 *  Switch  HALT    SimH          Function
		 *  ------  ----    ----          ---------
		 *  START   ON	    reset         INITIALIZE
		 *  START   OFF	    run <r_adrsc> INITIALIZE, and start processor operation at R_ADRSC
		 *  CONT    ON	    step 1        execute next single step
		 *  CONT    OFF	    cont          continue execution at PC
		 *
		 *  The 11/40 has no BOOT switch.
		 */
		if (action_switch == _this->switch_START) {
			// START has actions on rising AND falling edge!
			if (_this->switch_START->value && _this->switch_HALT->value) { // rising edge and HALT: INITIALIZE = SimH "RESET"
				sprintf(_this->realcons->simh_cmd_buffer, "reset all\n");
			}
			if (_this->switch_START->value && !_this->switch_HALT->value) {
				// rising edge and not HALT: will start on falling edge
				// but CONSOLE LED goes of now! Too difficult ....
				// console_led_prestart_off = 1;
			}
			if (!_this->switch_START->value && !_this->switch_HALT->value) {
				// falling edge and not HALT: start
				// INITIALIZE, and start processor operation at R_ADRSC
				sprintf(_this->realcons->simh_cmd_buffer, "run %o\n",
					_this->console_address_register);
				// flash with DATA LEDs
				_this->realcons->timer_running_msec[TIMER_DATA_FLASH] =
					_this->realcons->service_cur_time_msec + TIME_DATA_FLASH_MS;
			}
		}
		else if (action_switch == _this->switch_CONT && _this->switch_HALT->value) {
			// single step = SimH "STEP 1"
			sprintf(_this->realcons->simh_cmd_buffer, "step 1\n");
		}
		else if (action_switch == _this->switch_CONT && !_this->switch_HALT->value) {
			// continue =  SimH "CONT"
			sprintf(_this->realcons->simh_cmd_buffer, "cont\n");
		}

	} // action_switch

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

	if (_this->realcons->lamp_test) {
#ifdef OLD
		_this->led_ADDRESS->value = 0x3ffff;
		_this->led_DATA->value = 0xffff;
		_this->led_RUN->value = 1;
		_this->led_BUS->value = 1;
		_this->led_USER->value = 1;
		_this->led_PROCESSOR->value = 1;
		_this->led_CONSOLE->value = 1;
		_this->led_VIRTUAL->value = 1;
#endif
	}
	else {

		// ADRESS always shows BUS Adress register.
		_this->led_ADDRESS->value = SIGNAL_GET(cpusignal_memory_address_phys_register);
		//	if (_this->realcons->debug)
		//		printf("led_ADDRESS=%o, ledDATA=%o\n", (unsigned) _this->led_ADDRESS->value,
		//				(unsigned) _this->led_DATA->value);

		// DATA - CPU intern mux output ... a lot of different things.
		// see realcons_console_pdp11_40_set_machine_state()
		// state transition is signaled by SimH over machine_set_state()
		// DATA shows intern 11/40 processor DMUX
		if (_this->realcons->timer_running_msec[TIMER_DATA_FLASH])
			// all LEDs pulse ON after DEPOSIT, LOADADR etc
			_this->led_DATA->value = 0xffff;
		else
			_this->led_DATA->value = _this->DMUX;

		// BUS: 1 if device or CPU accesses the UNIBUS
		// PROC: the processor is accessing the UNIBUS
		// both are set directly in set_state()

		_this->led_CONSOLE->value = console_mode;

		_this->led_USER->value = user_mode;

		// BUS and PROCESSOR are ON in console mode
		// see http://www.youtube.com/watch?v=iIsZVqhaneo
		if (console_mode) {
			_this->led_BUS->value = 1;
			_this->led_PROCESSOR->value = 1;
		}

		// ADDRESS displays virtual address: true if processor runs
		_this->led_VIRTUAL->value = !console_mode;

		// RUN:
		// bright on HALT, off after RESET, "faint glow" in normal machine operation,
		if (SIGNAL_GET(cpusignal_run)) {
			if (_this->run_state == RUN_STATE_RESET || _this->run_state == RUN_STATE_WAIT)
				// current opcode is a RESET: RUN OFF
				_this->led_RUN->value = 0;
			else
				// Running: ON 1/2 of the time => flickering and "faint glow"
				_this->led_RUN->value = !(_this->realcons->service_cycle_count & 1);
		}
		else
			_this->led_RUN->value = 1; // CPU stop =) consoel mode alyways on
	} // if ! lamptest

	return SCPE_OK;
}

/*
 * start 1 sec test sequence.
 * - lamps ON for 1 sec
 * - print state of all switches
 */
int realcons_console_pdp11_40_test(realcons_console_logic_pdp11_40_t *_this, int arg)
{
	// send end time for test: 1 second = curtime + 1000
	// lamp test is set in service()
	_this->realcons->timer_running_msec[TIMER_TEST] = _this->realcons->service_cur_time_msec
		+ TIME_TEST_MS;

	realcons_printf(_this->realcons, stdout, "Verify lamp test!\n");
	realcons_printf(_this->realcons, stdout, "Switch SR        = %llo\n", _this->switch_SR->value);
	realcons_printf(_this->realcons, stdout, "Switch LOAD ADRS = %llo\n",
		_this->switch_LOAD_ADRS->value);
	realcons_printf(_this->realcons, stdout, "Switch EXAM      = %llo\n",
		_this->switch_EXAM->value);
	realcons_printf(_this->realcons, stdout, "Switch CONT      = %llo\n",
		_this->switch_CONT->value);
	realcons_printf(_this->realcons, stdout, "Switch HALT      = %llo\n",
		_this->switch_HALT->value);
	realcons_printf(_this->realcons, stdout, "Switch START     = %llo\n",
		_this->switch_START->value);
	realcons_printf(_this->realcons, stdout, "Switch DEPOSIT   = %llo\n",
		_this->switch_DEPOSIT->value);
	return 0; // OK
}

#endif // USE_REALCONS
