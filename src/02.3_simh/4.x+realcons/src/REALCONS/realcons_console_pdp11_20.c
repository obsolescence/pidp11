/* realcons_pdp11_20.c : Logic for the specific 11/20 panel

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

   21-Mar-2018	JH	Reset (HALT+START) load PC, merged with current SimH
   23-Apr-2016  JH  cloned from 11/70


 Inputs:
 - cpu state (PDP11 as set by SimH)
 - blinkenlight API panel input (switches)

 Outputs:
 - cpu state (as checked by SimH, example: HALT state)
 - blinkenlight API panel outputs (lamps)
 - SimH command string

 Class hierarchy:
 base panel = generic machine with properties requiered by SimH
 everthing accessed in scp.c is "base"

 inherited pdp11_20 (or other architecure)
 an actual Panel has an actual machine state.

 (For instance, a 11/05 has almost the same machine state,
 but quite a different panel)

 */
#ifdef USE_REALCONS

#include <assert.h>

#include "sim_defs.h"
#include "realcons.h"
#include "realcons_console_pdp11_20.h"

// indexes of used general timers
#define TIMER_TEST	0
#define TIMER_DATA_FLASH	1

#define TIME_TEST_MS		3000	// self test terminates after 3 seconds
#define TIME_DATA_FLASH_MS	0		// do DAT l3ds flash on 11/20?
// the LEDs behaves a bit difficult, so distinguish these states:
#define RUN_STATE_HALT	0
#define RUN_STATE_RESET	1
#define RUN_STATE_WAIT	2
#define RUN_STATE_RUN		3


/*
 * SimH gives not the CPU registers, if their IO page addresses are exam/deposit
 * So convert addr -> reg, and revers in the *_operator_reg_exam_deposit()
 */
static t_addr realcons_console_pdp11_20_addr_inc(t_addr addr)
{
    // no 1-byte inc for register space?
//    if (addr >= 0777700 && addr <= 0777707)
//        addr += 1; // inc to next register
//    else
        addr += 2; // inc by word
    addr &= 017777777; // trunc to 22 bit
    return addr;
}

// generate SimH expression for panel address
// - result string to be used in "sim>" exam or deposit cmd
// - translate memory addr to register, if CPU is accessed
// translate 2218bit 11/20 IOpage addr to 22bit SimH address
static char *realcons_console_pdp11_20_addr_panel2simh(t_addr panel_addr)
{
    static char buff[40];

        // physical 18 bit address
        switch (panel_addr) {
        case 017777700:
            return "r0";
        case 017777701:
            return "r1";
        case 017777702:
            return "r2";
        case 017777703:
            return "r3";
        case 017777704:
            return "r4";
        case 017777705:
            return "r5";
        case 017777706:
            return "sp";
        case 017777707:
            return "pc";
        default:
            panel_addr &= ~1; // clear LSB, according to DEC doc
            sprintf(buff, "%o", panel_addr);
            return buff;
        }
}

/* PDP-11/20 is 16 bit, SimH is 22 bit
    Internal the console address uses a 18 bit register.
   "The processor ignores bits 17 and 16; these switches may be set to either position.
    For addresses using bits 17 and 16, these bits are set within the processor if bits 15, 14, and 13 are set."
    expand a 16 bit address to 22 bit: bits 21:16 = 11, if bits 15,14,13 =111 (IOpage)
    */
static t_addr encode_SR_addr_16_to_22(t_addr sr_addr) {
    sr_addr &= 0177777; // trunc to 16
    if ((sr_addr & 0160000) == 0160000) // IO page
        return 017600000 | sr_addr; // 18 bit io page addr
    else
        return sr_addr;
}

// if a 22 bit SimH addr is to be displayed on the 18 bit address LEDs
static t_addr encode_SimH_addr_22_to_18(t_addr addr) {
    return addr & 0777777;
}


/* make a 16 bit value from the 18 bit SR switches
 " The processor ignores bits 17 and 16; these switches may be set to either position."
 */
static t_value encode_SR_18_to_16(t_value sr_val) {
    return sr_val & 0177777; // trunc to 16
}



/*
 * constructor / destructor
 */
realcons_console_logic_pdp11_20_t *realcons_console_pdp11_20_constructor(realcons_t *realcons)
{
    realcons_console_logic_pdp11_20_t *_this;

    _this = (realcons_console_logic_pdp11_20_t *) malloc(sizeof(realcons_console_logic_pdp11_20_t));
    _this->realcons = realcons; // connect to parent
    _this->run_state = 0;
    return _this;
}

void realcons_console_pdp11_20_destructor(realcons_console_logic_pdp11_20_t *_this)
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
// assume "realcons_console_logic_pdp11_20_t *_this" in context
#define SIGNAL_SET(signal,value) REALCONS_SIGNAL_SET(_this,signal,value)
#define SIGNAL_GET(signal) REALCONS_SIGNAL_GET(_this,signal)

void realcons_console_pdp11_20_event_connect(realcons_console_logic_pdp11_20_t *_this)
{
    // set panel mode to "powerless". all lights go off,
    // On Java panels the power switch should flip to the ON position
    realcons_power_mode(_this->realcons, 1);
}

void realcons_console_pdp11_20_event_disconnect(realcons_console_logic_pdp11_20_t *_this)
{
    // set panel mode to "powerless". all lights go off,
    // On Java panels the power switch should flip to the OFF position
    realcons_power_mode(_this->realcons, 0);
}

void realcons_console_pdp11_20__event_opcode_any(realcons_console_logic_pdp11_20_t *_this)
{
    // other opcodes executed by processor
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_20__event_opcode_any\n");
    // SIGNAL_SET(cpu_is_running, 1);
    // after any opcode: ADDR shows PC, DATA shows IR = opcode
    SIGNAL_SET(cpusignal_memory_address_phys_register, SIGNAL_GET(cpusignal_PC));
    _this->DATA_reg = SIGNAL_GET(cpusignal_DATAPATH_shifter); // show internal data

    // opcode fetch: ALU_output already set
    SIGNAL_SET(cpusignal_memory_data_register, SIGNAL_GET(cpusignal_instruction_register));
    // SIGNAL_SET(cpusignal_bus_register, SIGNAL_GET(cpusignal_instruction_register));
    _this->run_state = RUN_STATE_RUN;
}

void realcons_console_pdp11_20__event_opcode_halt(realcons_console_logic_pdp11_20_t *_this)
{
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_20__event_opcode_halt\n");
//	SIGNAL_SET(cpusignal_console_halt, 1);
    SIGNAL_SET(cpusignal_memory_address_phys_register, SIGNAL_GET(cpusignal_PC)-2);
    // what is in the unibus interface on HALT? IR == HALT Opcode?
    // SIGNAL_SET(cpusignal_busregister, ...)
    _this->DATA_reg = SIGNAL_GET(cpusignal_R0); // show R0 on halt
    _this->run_state = RUN_STATE_HALT; // show signal R0
}

void realcons_console_pdp11_20__event_opcode_reset(realcons_console_logic_pdp11_20_t *_this)
{
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_20__event_opcode_reset\n");
//	SIGNAL_SET(cpusignal_console_halt, 0);
    SIGNAL_SET(cpusignal_memory_address_phys_register, SIGNAL_GET(cpusignal_PC));
    // what is in the unibus interface on RESET? IR == RESET Opcode ?
    // SIGNAL_SET(cpusignal_busregister, ...)
    // _this->DMUX = SIGNAL_GET(cpusignal_R0);
    _this->run_state = RUN_STATE_RESET;

    // duration of RESET on 11/20 unclear. do 70ms, as on 11/40
    realcons_ms_sleep(_this->realcons, 70); // keep realcons logic active


}

void realcons_console_pdp11_20__event_opcode_wait(realcons_console_logic_pdp11_20_t *_this)
{
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_20__event_opcode_wait\n");
//	SIGNAL_SET(cpusignal_console_halt, 0);
    SIGNAL_SET(cpusignal_memory_address_phys_register, SIGNAL_GET(cpusignal_PC));
    SIGNAL_SET(cpusignal_memory_data_register, SIGNAL_GET(cpusignal_instruction_register));
    //SIGNAL_SET(cpusignal_bus_register, SIGNAL_GET(cpusignal_instruction_register));
    _this->run_state = RUN_STATE_WAIT; // RUN led off
}

void realcons_console_pdp11_20__event_run_start(realcons_console_logic_pdp11_20_t *_this)
{
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_20__event_run_start\n");
    if (_this->switch_HALT->value)
        return; // do not accept RUN command
    // set property RUNMODE, so SimH can read it back
//	SIGNAL_SET(cpusignal_console_halt, 0); // running
    _this->run_state = RUN_STATE_RUN;
}

void realcons_console_pdp11_20__event_step_start(realcons_console_logic_pdp11_20_t *_this)
{
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_20__event_step_start\n");
//	SIGNAL_SET(cpusignal_console_halt, 0); // running
}

void realcons_console_pdp11_20__event_operator_halt(realcons_console_logic_pdp11_20_t *_this)
{
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_20__event_operator_halt\n");
    SIGNAL_SET(cpusignal_memory_address_phys_register, SIGNAL_GET(cpusignal_PC) - 2);

//	SIGNAL_SET(cpusignal_console_halt, 1);
//    SIGNAL_SET(cpusignal_memory_address_register, SIGNAL_GET(cpusignal_PC));
    // SIGNAL_SET(cpusignal_DATAPATH_shifter, SIGNAL_GET(cpusignal_PSW));
    _this->run_state = RUN_STATE_HALT;
}

void realcons_console_pdp11_20__event_step_halt(realcons_console_logic_pdp11_20_t *_this)
{
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_20__event_step_halt\n");
//	SIGNAL_SET(cpusignal_console_halt, 1);
    SIGNAL_SET(cpusignal_memory_address_phys_register, SIGNAL_GET(cpusignal_PC)-2);
    // SIGNAL_SET(cpusignal_DATAPATH_shifter, SIGNAL_GET(cpusignal_PSW));
    _this->run_state = RUN_STATE_HALT;
}

// this is called, when "exam" or "deposit" is entered over the
// sim> command prompt by user or as result of an EXAM or DEPOSIT panel switch.
// if ADDR SELECT knob is VIRUAL, the virtual address for the
// SimH physical address should be displayed.
// This is impossible: not every physical addr has a virtual representation
// for given address space (KERNEL,SUPER,USER, I/D)
// So simpy DO NOT UPDATE panel regsiter if KNBOB is virtual,
// then at least the key switch values remain in ADRR LEDs
void realcons_console_pdp11_20__event_operator_exam_deposit(
        realcons_console_logic_pdp11_20_t *_this)
{
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_20__event_operator_exam\n");
        _this->DATA_reg =  SIGNAL_GET(cpusignal_memory_data_register);
}

// Called if Realcons or a user typed "examine <regname>" or "deposit <regname> value".
// Realcons accesses only the CPU registers by name, so only these are recognized here
// see realcons_console_pdp11_20_addr_panel2simh() !
void realcons_console_pdp11_20__event_operator_reg_exam_deposit(
        realcons_console_logic_pdp11_20_t *_this)
{
    char *regname = SIGNAL_GET(cpusignal_register_name); // alias
    t_addr addr = 0;

    if (_this->realcons->debug)
        printf("realcons_console_pdp11_20__event_operator_reg_exam_deposit\n");
    // exam on SimH console sets also console address (like LOAD ADDR)
    // convert register into UNIBUS address
    if (!strcasecmp(regname, "r0"))
        addr = 017777700;
    else if (!strcasecmp(regname, "r1"))
        addr = 017777701;
    else if (!strcasecmp(regname, "r2"))
        addr = 017777702;
    else if (!strcasecmp(regname, "r3"))
        addr = 017777703;
    else if (!strcasecmp(regname, "r4"))
        addr = 017777704;
    else if (!strcasecmp(regname, "r5"))
        addr = 017777705;
    else if (!strcasecmp(regname, "sp"))
        addr = 017777706;
    else if (!strcasecmp(regname, "pc"))
        addr = 017777707;
    if (addr) {
        SIGNAL_SET(cpusignal_memory_address_phys_register, addr);
        realcons_console_pdp11_20__event_operator_exam_deposit(_this);
    } // other register accesses are ignored by the panel
}


// Called after CPU has been reinitialized
void realcons_console_pdp11_20__event_cpu_reset(
        realcons_console_logic_pdp11_20_t *_this) {
    if (_this->realcons->debug)
        printf("realcons_console_pdp11_20__event_cpu_reset\n");

    // TODO: load PC with result from LOAD ADDR, for "HALT+START" reset ?
    // This does NOT work:
//    SIGNAL_SET(cpusignal_PC, SIGNAL_GET(cpusignal_memory_address_phys_register)) ;
}


void realcons_console_pdp11_20_interface_connect(realcons_console_logic_pdp11_20_t *_this,
        realcons_console_controller_interface_t *intf, char *panel_name)
{
    intf->name = panel_name;
    intf->constructor_func =
            (console_controller_constructor_func_t) realcons_console_pdp11_20_constructor;
    intf->destructor_func =
            (console_controller_destructor_func_t) realcons_console_pdp11_20_destructor;
    intf->reset_func = (console_controller_reset_func_t) realcons_console_pdp11_20_reset;
    intf->service_func = (console_controller_service_func_t) realcons_console_pdp11_20_service;
    intf->test_func = (console_controller_test_func_t) realcons_console_pdp11_20_test;

    intf->event_connect = (console_controller_event_func_t) realcons_console_pdp11_20_event_connect;
    intf->event_disconnect =
            (console_controller_event_func_t) realcons_console_pdp11_20_event_disconnect;

    // connect pdp11 cpu signals end events to simulator and realcons state variables
    {
        // REALCONS extension in scp.c
        extern t_addr realcons_memory_address_phys_register; // REALCONS extension in scp.c

        extern t_value realcons_memory_data_register; // REALCONS extension in scp.c
        extern char *realcons_register_name; // pseudo: name of last accessed register
        extern int realcons_console_halt; // 1: CPU halted by realcons console
        extern volatile t_bool sim_is_running; // global in scp.c

        extern int32 R[8]; // working registers, global in pdp11_cpu.c
        extern int32 saved_PC;
        extern int32 SR; // switch register, global in pdp11_cpu_mod.c
        //extern t_addr realcons_console_address_register; // set by LOAD ADDR
        extern t_value realcons_DATAPATH_shifter; // output of ALU
        extern t_value realcons_IR; // buffer for instruction register (opcode)
        extern t_value realcons_PSW; // buffer for program status word

        realcons_console_halt = 0;

        // from scp.c
        _this->cpusignal_memory_address_phys_register = &realcons_memory_address_phys_register;
        _this->cpusignal_register_name = &realcons_register_name; // pseudo: name of last accessed register
        _this->cpusignal_memory_data_register = &realcons_memory_data_register; // BAR
        _this->cpusignal_console_halt = &realcons_console_halt;

        // from pdp11_cpu.c
        // is "sim_is_running" indeed identical with our "cpu_is_running" ?
        // may cpu stops, but some device are still serviced?
        _this->cpusignal_run = &(sim_is_running);

        _this->cpusignal_DATAPATH_shifter = &realcons_DATAPATH_shifter; // not used
        //_this->cpusignal_console_address_register = &realcons_console_address_register; // not used

        // set by LOAD ADDR, on all PDP11's
        // signal from realcons console to CPU: 1=HALTed
        // oder gleich "&(_switch_HALT->value)?"
        _this->cpusignal_instruction_register = &realcons_IR;
        _this->cpusignal_PSW = &realcons_PSW;
        _this->cpusignal_R0 = (t_value*)&(R[0]); // R: global of pdp11_cpu.c
        _this->cpusignal_PC = &saved_PC; // R[7] not valid in console mode
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
                (console_controller_event_func_t) realcons_console_pdp11_20__event_run_start;
        realcons_event_step_start =
                (console_controller_event_func_t) realcons_console_pdp11_20__event_step_start;
        realcons_event_operator_halt =
                (console_controller_event_func_t) realcons_console_pdp11_20__event_operator_halt;
        realcons_event_step_halt =
                (console_controller_event_func_t) realcons_console_pdp11_20__event_step_halt;
        realcons_event_operator_exam =
                realcons_event_operator_deposit =
                        (console_controller_event_func_t) realcons_console_pdp11_20__event_operator_exam_deposit;
        realcons_event_operator_reg_exam =
                realcons_event_operator_reg_deposit =
                        (console_controller_event_func_t) realcons_console_pdp11_20__event_operator_reg_exam_deposit;
		realcons_event_cpu_reset =
			   (console_controller_event_func_t) realcons_console_pdp11_20__event_cpu_reset ;
        realcons_event_opcode_any =
                (console_controller_event_func_t) realcons_console_pdp11_20__event_opcode_any;
        realcons_event_opcode_halt =
                (console_controller_event_func_t) realcons_console_pdp11_20__event_opcode_halt;
        realcons_event_opcode_reset =
                (console_controller_event_func_t) realcons_console_pdp11_20__event_opcode_reset;
        realcons_event_opcode_wait =
                (console_controller_event_func_t) realcons_console_pdp11_20__event_opcode_wait;
    }
}




// setup first state
t_stat realcons_console_pdp11_20_reset(realcons_console_logic_pdp11_20_t *_this)
{
    _this->realcons->simh_cmd_buffer[0] = '\0';
    //SIGNAL_SET(cpusignal_console_address_register, 0);
    SIGNAL_SET(cpusignal_DATAPATH_shifter, 0); // else DATA trash is shown before first EXAM
    _this->autoinc_addr_action_switch = NULL; // not active
    _this->DATA_reg = 0 ;

    /*
     * direct links to all required controls.
     * Also check of config file
     */
    if (!(_this->keyswitch_POWER = realcons_console_get_input_control(_this->realcons, "POWER")))
        return SCPE_NOATT;
    if (!(_this->keyswitch_PANEL_LOCK = realcons_console_get_input_control(_this->realcons, "PANEL_LOCK")))
        return SCPE_NOATT;

    if (!(_this->switch_SR = realcons_console_get_input_control(_this->realcons, "SR")))
        return SCPE_NOATT;
    if (!(_this->switch_LOAD_ADDR = realcons_console_get_input_control(_this->realcons, "LOAD_ADDR")))
        return SCPE_NOATT;
    if (!(_this->switch_EXAM = realcons_console_get_input_control(_this->realcons, "EXAM")))
        return SCPE_NOATT;
    if (!(_this->switch_CONT = realcons_console_get_input_control(_this->realcons, "CONT")))
        return SCPE_NOATT;
    if (!(_this->switch_HALT = realcons_console_get_input_control(_this->realcons, "HALT")))
        return SCPE_NOATT;
    if (!(_this->switch_SINST_SCYCLE = realcons_console_get_input_control(_this->realcons,
            "SCYCLE")))
        return SCPE_NOATT;
    if (!(_this->switch_START = realcons_console_get_input_control(_this->realcons, "START")))
        return SCPE_NOATT;
    if (!(_this->switch_DEPOSIT = realcons_console_get_input_control(_this->realcons, "DEPOSIT")))
        return SCPE_NOATT;

    if (!(_this->leds_ADDRESS = realcons_console_get_output_control(_this->realcons, "ADDRESS")))
        return SCPE_NOATT;
    if (!(_this->leds_DATA = realcons_console_get_output_control(_this->realcons, "DATA")))
        return SCPE_NOATT;
    if (!(_this->led_RUN = realcons_console_get_output_control(_this->realcons, "RUN")))
        return SCPE_NOATT;
    if (!(_this->led_BUS = realcons_console_get_output_control(_this->realcons, "BUS")))
        return SCPE_NOATT;
    if (!(_this->led_FETCH = realcons_console_get_output_control(_this->realcons, "FETCH")))
        return SCPE_NOATT;
    if (!(_this->led_EXEC = realcons_console_get_output_control(_this->realcons, "EXEC")))
        return SCPE_NOATT;
    if (!(_this->led_SOURCE = realcons_console_get_output_control(_this->realcons, "SOURCE")))
        return SCPE_NOATT;
    if (!(_this->led_DESTINATION = realcons_console_get_output_control(_this->realcons, "DESTINATION")))
        return SCPE_NOATT;
    if (!(_this->leds_ADDRESS_CYCLE = realcons_console_get_output_control(_this->realcons,
            "ADDRESS_CYCLE")))
        return SCPE_NOATT;
    return SCPE_OK;
}


// process panel state.
// operates on Blinkenlight_API panel structs,
// but RPC communication is done external
t_stat realcons_console_pdp11_20_service(realcons_console_logic_pdp11_20_t *_this)
{
    blinkenlight_panel_t *p = _this->realcons->console_model; // alias
    int panel_lock;
    int console_mode;

    blinkenlight_control_t *action_switch; // current action switch

    if (_this->keyswitch_POWER->value == 0 ) {
        SIGNAL_SET(cpusignal_console_halt, 1); // stop execution
        if (_this->keyswitch_POWER->value_previous == 1) {
            // Power switch transition to POWER OFF: terminate SimH
            // This is drastic, but will teach users not to twiddle with the power switch.
            // when panel is disconnected, panel mode goes to POWERLESS and power switch goes OFF.
            // But shutdown sequence is not initiated, because we're disconnected then.
            sprintf(_this->realcons->simh_cmd_buffer, "quit"); // do not confirm the quit with ENTER
        }
        // do nothing, if power is off. else cpusignal_console_halt may be deactivate by HALT switch
        return SCPE_OK;
    }

    /*
    LOCK - Same as POWER, except that the LOAD ADRS, EXAM, DEP, CONT, ENABLE/HALT, S
           INST/S BUS CYCLE and START switches are disabled. All other switches are
           operational.
    */
    panel_lock = (_this->keyswitch_PANEL_LOCK->value != 0) ;


    /* test time expired? */

    // STOP by activating HALT?
    if (!panel_lock && _this->switch_HALT->value && !SIGNAL_GET(cpusignal_console_halt)) {
        // must be done by SimH.pdp11_cpu.c!
    }

    // CONSOLE: processor accepts commands from console panel when HALTed
    console_mode = !SIGNAL_GET(cpusignal_run) || SIGNAL_GET(cpusignal_console_halt);

    /*************************************************************
     * eval switches
     * react on changed action switches
     */

    // fetch switch register
    SIGNAL_SET(cpusignal_switch_register, encode_SR_18_to_16((t_value)_this->switch_SR->value));

    // fetch HALT mode, must be sensed by simulated processor to produce state OPERATOR_HALT
    if (panel_lock)
        SIGNAL_SET(cpusignal_console_halt,0) ;
    else
        SIGNAL_SET(cpusignal_console_halt, (t_value )_this->switch_HALT->value);

    /* which command switch was activated? Process only one of these
     * "command switch" is NO term of 11/20 docs, but excluding logic seems useful */
    action_switch = NULL;
    if (!panel_lock) {
    if (!action_switch && _this->switch_LOAD_ADDR->value == 1
            && _this->switch_LOAD_ADDR->value_previous == 0)
        action_switch = _this->switch_LOAD_ADDR;
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
    }
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
        SIGNAL_SET(cpusignal_memory_address_phys_register,
            realcons_console_pdp11_20_addr_inc(SIGNAL_GET(cpusignal_memory_address_phys_register))); // BAR

        /* "The LOAD ADDR switch transfers the SWITCH REGISTER contents to the Bus Address Register (BAR) through a
            temporary location (TEMP) within the processor. This bus address, displayed in the ADDRESS REGISTER, provides
            an address for the console functions of EXAM, DEP, and START."*/
        if (action_switch == _this->switch_LOAD_ADDR) {
            SIGNAL_SET(cpusignal_memory_address_phys_register, // BAR
                    (realcons_machine_word_t ) encode_SR_addr_16_to_22(encode_SR_18_to_16((t_value)_this->switch_SR->value)));
            SIGNAL_SET(cpusignal_DATAPATH_shifter, 0); // 11/40 videos show: DATA is cleared

            if (_this->realcons->debug)
                printf("LOADADR %o\n", SIGNAL_GET(cpusignal_memory_address_phys_register));
            // flash with DATA LEDs on 11/40. 11/70 ?
            _this->realcons->timer_running_msec[TIMER_DATA_FLASH] =
                    _this->realcons->service_cur_time_msec + TIME_DATA_FLASH_MS;
        }

        if (action_switch == _this->switch_EXAM) {
//            t_addr pa; // physical address
            _this->autoinc_addr_action_switch = _this->switch_EXAM; // inc addr on next EXAM
            // generate simh "exam cmd"
            // fix octal, should use SimH-radix
            sprintf(_this->realcons->simh_cmd_buffer, "examine %s\n",
                realcons_console_pdp11_20_addr_panel2simh(SIGNAL_GET(cpusignal_memory_address_phys_register)));
        }

        if (action_switch == _this->switch_DEPOSIT) {
//            t_addr pa; // physical address
            unsigned dataval;
            dataval = (realcons_machine_word_t)encode_SR_18_to_16((t_value)_this->switch_SR->value); // trunc to switches 15..0
            _this->autoinc_addr_action_switch = _this->switch_DEPOSIT; // inc addr on next DEP

            // produce SimH cmd. fix octal, should use SimH-radix
            sprintf(_this->realcons->simh_cmd_buffer, "deposit %s %o\n",
                    realcons_console_pdp11_20_addr_panel2simh(
                            SIGNAL_GET(cpusignal_memory_address_phys_register)), dataval);
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
         *  The 11/20 has no BOOT switch.
         */

        /*"When ENABLE/HALT is set to ENABLE, depressing START provides a system dear operation and then begins processor
           operation. A LOAD ADDR operation pre-establishes the starting address.
           When ENABLE/HALT is set to HALT, depressing START provides a system clear (initialize) only. The processor
           does not start; the Bus Address Register is loaded from a temporary processor register (TEMP) which is usually
            pre-loaded by LOAD ADDR.
            This provides the only method of reading TEMP when it does not contain the LOAD ADDR value."
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
                // not documented, but start at virtual addr must be also be converted to physical
                sprintf(_this->realcons->simh_cmd_buffer, "run %o\n",
                        SIGNAL_GET(cpusignal_memory_address_phys_register) // always 22 bit physical ?
                                );
                // flash with DATA LEDs
                _this->realcons->timer_running_msec[TIMER_DATA_FLASH] =
                        _this->realcons->service_cur_time_msec + TIME_DATA_FLASH_MS;
            }
        } else if (action_switch == _this->switch_CONT && _this->switch_HALT->value) {
            // single step = SimH "STEP 1"
            sprintf(_this->realcons->simh_cmd_buffer, "step 1\n");
        } else if (action_switch == _this->switch_CONT && !_this->switch_HALT->value) {
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

    //	if (_this->realcons->debug)
    //		printf("led_ADDRESS=%o, ledDATA=%o\n", (unsigned) _this->led_ADDRESS->value,
    //				(unsigned) _this->led_DATA->value);

    /*"ADDRESS REGISTER:
     Displays the address in the bus address register (BAR) of the processor. This is the address last used by the
     processor on the bus.
     The BAR is 18 bits, allowing for future expansion. At present, the two most significant bits (A17, A16) are
     ordered according to the lower 16 bits; they are set only when bits A15, A14, and A13 are set. Addresses
     between 160000 and 177777, therefore, are translated to addresses between 760000 and 777777, respectively.
     When console switches are used, information shown on the ADDRESS REGISTER display is as follows:
     LOAD ADDR - the transferred SWITCH REGISTER information
     DEP or EXAM — indicates the bus address just deposited into or examined.
     S-INST or S-CYCLE — indicates the last processor bus address.
     During a programmed HALT or WAIT instruction, the ADDRESS REGISTER displays the address of the instruction.
     The program counter (PC) is the BAR value plus 2.
     During direct memory access (DMA) operations, the processor is not involved in the data transfer functions,
     and the address displayed in the ADDRESS REGISTER is not that of the last bus operation.
     "*/
        _this->leds_ADDRESS->value = SIGNAL_GET(cpusignal_memory_address_phys_register);
        // all logic must influence cpusignal_memory_address_register !
        // cpusignal_memory_address_register is updated by simulator


    /* "DATA: When console switches are used, information shown on the DATA display is as follows:
        LOAD ADDR - no indication
        DEP - the SWITCH REGISTER information just deposited. Note that the data and address correlated. The address
            is where this data was stored.
        EXAM - the information from the address examined - note address and data correlation.
        S-INST - when stepping through a program a single instruction at a time, there is no indication on the DATA
            display.
        S-CYCLE - the information last in the data paths (refer to flow diagram). Usually is a derivative of last bus
            data.
        During HALT and WAIT instructions, information shown on DATA display is as follows:
        WAIT - The RUN light is on, no indication on DATA display.
        HALT - when bus control is transferred to the console on a HALT instruction, processor register RO is
            displayed. This allows program identification of halts.
        During direct memory access (EMA) operations, the processor is not involved in data transfer functions.
        Therefore, the data displayed in the DATA display is not that of the last bus operation."
    */
    if (_this->realcons->timer_running_msec[TIMER_DATA_FLASH])
        // all LEDs pulse ON after DEPOSIT, LOADADR etc
        _this->leds_DATA->value = 0xfffff;
    else
        _this->leds_DATA->value = _this->DATA_reg;


    // RUN:
    /*"When the RUN indicator is on, the processor dock is running. The processor has control of the bus and is
       operating on an instruction.
       When the RUN indicator is off, the processor is waiting for an asynchronous peripheral data response; or the
       processor has given up bus control to the console or to a peripheral.
       During normal machine operation the RUN light will flicker on and off (indicated by a faint glow).
       Special situations exist for programmed HALT and WAIT instructions:
       WAIT — the RUN light is completely on
       HALT — the RUN light is off
       During machine operation, with S-INST or S-CYCLE control transferred to the console, the RUN light is not on."
     */

    // bright on HALT, off after RESET, "faint glow" in normal machine operation,
    if (SIGNAL_GET(cpusignal_run)) {
        if (_this->run_state == RUN_STATE_RESET || _this->run_state == RUN_STATE_HALT )
            _this->led_RUN->value = 0; // RESET undefined by docs
        else if (_this->run_state == RUN_STATE_WAIT)
            _this->led_RUN->value = 1;
        else
            // Running: ON 1/4 of the time => flickering and "faint glow": 50%
            _this->led_RUN->value = !(_this->realcons->service_cycle_count % 2);
    } else
        // not running.
        _this->led_RUN->value = 0;

    /*" When the BUS indicator is on, a peripheral device (other than the console) is controlling the bus.
        When both the BUS and RUN indicators are off, the bus control has been transferred to the console.
        The bus light probably is never seen except when there is a bus malfunction or when a peripheral holds the bus
        for excessive periods of time. Refer to Paragraph 3.3 for further discussion of BUS and RUN indicator
        combinations.
        During machine operation with S-INST or S-CYCLE, control is transferred to the console and the BUS indicator
        is not on.
      FETCH: "When on, the processor is in the FETCH major state and is obtaining an instruction.
        During the Fetch major state, only FETCH and RUN indicators are on if no NPRs are honored."

       EXEC: When on the processor is in the EXECUTE major state. The processor performs the action specified by the
        instruction. (HALT, WAIT, and trap instructions are executed in Service.)
        During the Execute major state, only EXEC and RUN indicators are on if no NPRs are honored.

        SOURCE: When on, the processor is in the Source major state and is obtaining source operand data. The processor
        calculates source address data as indicated by cycles of the ADDRESS lights.
        During the Source major state, SOURCE and RUN indicators are both on; ADDRESS lights may be on in various
        combinations. The BUS indicator is off if no NPRs are honored.


        DESTINATION: When on, the processor is in the Destination major state and is obtaining destination operand data. The
        processor calculates destination address data as indicated by cycles of the ADDRESS lights.
        During the Destination major state, DESTINATION and RUN indicators are both on; ADDRESS lights may be on in
        various combinations. The BUS indicator is off if no NPRs are honored.

        ADDRESS: When lit, indicate bus cycles used to obtain address data during Source and Destination major states.
        The 2-bit binary code indicates which address cycle (1, 2, or 3) the machine is in during the Source or
        Destination major state.
        Whenever either one or both ADDRESS lights are lit, either the SOURCE or DESTINATION indicator is on.
        The BUS indicator is off if no NPRs are honored.
        "*/
    // if CPU is running, simulate some plausible light patterns (SimH does not simulate single cycles)
    // FETCH 50%, EXEC 33%, SOURCE 25%, DESTINATION 15%, ADDRESS: 20% 1 access, 5% 2 accesses, 0 % 3 accesses
    _this->led_BUS = 0; // well ...
    if (SIGNAL_GET(cpusignal_run) && _this->run_state == RUN_STATE_RUN) {
        int n;
        _this->led_FETCH->value = !(_this->realcons->service_cycle_count % 2);
        _this->led_EXEC->value = !(_this->realcons->service_cycle_count % 3);
            _this->led_SOURCE->value = !(_this->realcons->service_cycle_count % 4);
                _this->led_DESTINATION->value = !(_this->realcons->service_cycle_count % 6);
                n = 0;
                if (!(_this->realcons->service_cycle_count % 5))
                    n = 1;
                else if (!(_this->realcons->service_cycle_count % 20))
                    n = 2;
                    _this->leds_ADDRESS_CYCLE->value = n ;
    }
    else
        _this->led_FETCH->value = _this->led_EXEC->value = _this->led_SOURCE->value = _this->led_DESTINATION->value = _this->leds_ADDRESS_CYCLE->value = 0;

    return SCPE_OK;
}

/*
 * start 1 sec test sequence.
 * - lamps ON for 1 sec: TIME_TEST_MS
 * - print state of all switches
 */
int realcons_console_pdp11_20_test(realcons_console_logic_pdp11_20_t *_this, int arg)
{
    // send end time for test: 1 second = curtime + 1000
    // lamp test is set in service()
    _this->realcons->timer_running_msec[TIMER_TEST] = _this->realcons->service_cur_time_msec
            + TIME_TEST_MS;

    realcons_printf(_this->realcons, stdout, "Verify lamp test!\n");
    realcons_printf(_this->realcons, stdout, "Switch SR             = %llo\n",
            _this->switch_SR->value);
    realcons_printf(_this->realcons, stdout, "Switch LOAD ADDR      = %llo\n",
            _this->switch_LOAD_ADDR->value);
    realcons_printf(_this->realcons, stdout, "Switch EXAM           = %llo\n",
            _this->switch_EXAM->value);
    realcons_printf(_this->realcons, stdout, "Switch CONT           = %llo\n",
        _this->switch_CONT->value);
    realcons_printf(_this->realcons, stdout, "Switch ENABLE/HALT    = %llo\n",
        _this->switch_HALT->value);
    realcons_printf(_this->realcons, stdout, "Switch S-INST/S-CYCLE = %llo\n",
        _this->switch_SINST_SCYCLE->value);
    realcons_printf(_this->realcons, stdout, "Switch START          = %llo\n",
        _this->switch_START->value);
    realcons_printf(_this->realcons, stdout, "Switch DEPOSIT        = %llo\n",
        _this->switch_DEPOSIT->value);
    return 0; // OK
}


#endif // USE_REALCONS
