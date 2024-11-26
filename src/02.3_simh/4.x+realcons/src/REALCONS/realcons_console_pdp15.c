/* realcons_pdp15.c:  Logic for the PDP-15 panel

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

 15-Aug-2016  JH      fix lamps_api_states_active, new address_switch_register
 14-Aug-2016  JH      added fake "major state" and "time state" light patterns,
                      DATA switches set SR register
 09-Aug-2016  JH      fix BANK/PAGE mode: BANK MODE switch is SimH BANKM_INIT / memm_init
 added PI/API lamps functions
 07-Aug-2016  JH      fix BANK/PAGE mode
 18-Jun-2016  JH      created


 Inputs:
 - realcons_machine state (PDP15 as set by SimH)
 - blinkenlight API panel input (switches)

 Outputs:
 - realcons_machine state (as checked by SimH, example: HALT state)
 - blinkenlight API panel outputs (lamps)
 - SimH command string

 Class hierarchy:
 base panel = generic machine with properties requiered by SimH
 everthing accessed in scp.c is "base"

 inherited pdp15 (or other architecure)
 an actual Panel has an actual machine state.

 (For instance, a 8/L has almost the same machine state,
 but quite a different panel)

 */
#include <assert.h>

#include "sim_defs.h"
#include "realcons.h"
#include "realcons_console_pdp15.h"

// SHORTER macros for signal access
// assume "realcons_console_logic_pdp15_t *_this" in context
#define SIGNAL_SET(signal,value) REALCONS_SIGNAL_SET(_this,signal,value)
#define SIGNAL_GET(signal) REALCONS_SIGNAL_GET(_this,signal)

// indexes of used general timers
#define TIMER_TEST	0
#define TIMER_DATA_FLASH	1

#define TIME_TEST_MS		3000	// self test terminates after 3 seconds

#define DATA18BITMASK 0777777

/*
 * constructor / destructor
 */
realcons_console_logic_pdp15_t *realcons_console_pdp15_constructor(realcons_t *realcons)
{
    realcons_console_logic_pdp15_t *_this;

    _this = (realcons_console_logic_pdp15_t *) malloc(sizeof(realcons_console_logic_pdp15_t));
    _this->realcons = realcons; // connect to parent
    _this->run_state = 0;

    return _this;
}

void realcons_console_pdp15_destructor(realcons_console_logic_pdp15_t *_this)
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

void realcons_console_pdp15__event_connect(realcons_console_logic_pdp15_t *_this)
{
//	realcons_console_clear_output_controls(_this->realcons) ; // every LED to state 0
    // set panel mode to normal
    // On Java panels the powerbutton flips to the ON position.
    realcons_power_mode(_this->realcons, 1);
}

void realcons_console_pdp15__event_disconnect(realcons_console_logic_pdp15_t *_this)
{
//	realcons_console_clear_output_controls(_this->realcons); // every LED to state 0
    // set panel mode to "powerless". all lights go off,
    // On Java panels the powerbutton flips to the OFF position
    realcons_power_mode(_this->realcons, 0);
}

void realcons_console_pdp15__event_opcode_any(realcons_console_logic_pdp15_t *_this)
{
#ifdef TODO
    // other opcodes executed by processor
    if (_this->realcons->debug)
    printf("realcons_console_pdp15__event_opcode_any\n");
    // after any opcode: ADDR shows PC, DATA shows IR = opcode
    SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_pdp11.PC));
    _this->DMUX = SIGNAL_GET(signals_cpu_pdp11.instruction_register);
    // show opcode
    _this->led_BUS->value = 1;// UNIBUS cycles ....
    _this->led_PROCESSOR->value = 1;// ... by processor
    _this->run_state = RUN_STATE_RUN;
#endif
}

void realcons_console_pdp15__event_opcode_halt(realcons_console_logic_pdp15_t *_this)
{
#ifdef TODO
    if (_this->realcons->debug)
    printf("realcons_console_pdp15__event_opcode_halt\n");
    SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_pdp11.PC));
    _this->DMUX = SIGNAL_GET(signals_cpu_pdp11.R0);
    _this->led_BUS->value = 0; // no UNIBUS cycles any more
    _this->led_PROCESSOR->value = 0;// processor stop
    _this->run_state = RUN_STATE_HALT;
#endif
}

void realcons_console_pdp15__event_opcode_reset(realcons_console_logic_pdp15_t *_this)
{
#ifdef TODO
    if (_this->realcons->debug)
    printf("realcons_console_pdp15__event_opcode_reset\n");
    SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_pdp11.PC));
    _this->DMUX = SIGNAL_GET(signals_cpu_pdp11.R0);
    _this->led_BUS->value = 1; // http://www.youtube.com/watch?v=iIsZVqhaneo
    _this->led_PROCESSOR->value = 1;
    _this->run_state = RUN_STATE_RESET;
#endif
}

void realcons_console_pdp15__event_opcode_wait(realcons_console_logic_pdp15_t *_this)
{
#ifdef TODO
    if (_this->realcons->debug)
    printf("realcons_console_pdp15__event_opcode_wait\n");
    SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_pdp11.PC));
    _this->DMUX = SIGNAL_GET(signals_cpu_pdp11.instruction_register);
    _this->led_BUS->value = 0; // no UNIBUS cycles any more
    _this->led_PROCESSOR->value = 0;// processor idle
    _this->run_state = RUN_STATE_WAIT;// RUN led off
#endif
}

void realcons_console_pdp15__event_run_start(realcons_console_logic_pdp15_t *_this)
{
#ifdef TODO
    if (_this->realcons->debug)
    printf("realcons_console_pdp15__event_run_start\n");
    if (_this->switch_HALT->value)
    return; // do not accept RUN command
    // set property RUNMODE, so SimH can read it back
    _this->led_BUS->value = 1;// start accessing UNIBUS cycles
    _this->led_PROCESSOR->value = 1;// processor active
    _this->run_state = RUN_STATE_RUN;
#endif
}

void realcons_console_pdp15__event_step_start(realcons_console_logic_pdp15_t *_this)
{
#ifdef TODO
    if (_this->realcons->debug)
    printf("realcons_console_pdp15__event_step_start\n");
    _this->led_BUS->value = 1; // start accessing UNIBUS cycles
    _this->led_PROCESSOR->value = 1;// processor active
#endif
}

// after single step and on operator halt the same happens
void realcons_console_pdp15__event_operator_step_halt(realcons_console_logic_pdp15_t *_this)
{
#ifdef TODO
    if (_this->realcons->debug)
    printf("realcons_console_pdp15__event_operator_step_halt\n");
    SIGNAL_SET(signals_cpu_generic.bus_address_register, SIGNAL_GET(signals_cpu_pdp11.PC));
    _this->DMUX = SIGNAL_GET(signals_cpu_pdp11.PSW);
    _this->led_BUS->value = 0; // no UNIBUS cycles any more
    _this->led_PROCESSOR->value = 0;// processor idle
    _this->run_state = RUN_STATE_HALT;
#endif
}
void realcons_console_pdp15__event_operator_exam_deposit(realcons_console_logic_pdp15_t *_this)
{
    // last manual (EXAM/DEPOST (Simh or console switches)
    SIGNAL_SET(cpusignal_operand_address_register, SIGNAL_GET(cpusignal_memory_address_phys_register));
}

void realcons_console_pdp15_interface_connect(realcons_console_logic_pdp15_t *_this,
        realcons_console_controller_interface_t *intf, char *panel_name)
{
    intf->name = panel_name;
    intf->constructor_func =
            (console_controller_constructor_func_t) realcons_console_pdp15_constructor;
    intf->destructor_func =
            (console_controller_destructor_func_t) realcons_console_pdp15_destructor;
    intf->reset_func = (console_controller_reset_func_t) realcons_console_pdp15_reset;
    intf->service_func = (console_controller_service_func_t) realcons_console_pdp15_service;
    intf->test_func = (console_controller_test_func_t) realcons_console_pdp15_test;
    intf->event_connect = (console_controller_event_func_t) realcons_console_pdp15__event_connect;
    intf->event_disconnect =
            (console_controller_event_func_t) realcons_console_pdp15__event_disconnect;

    // connect pdp8 cpu signals end events to simulator and realcons state variables
    {
        // REALCONS extension in scp.c
        extern t_addr realcons_memory_address_phys_register; // REALCONS extension in scp.c
        extern char *realcons_register_name; // pseudo: name of last accessed register
        extern t_value realcons_memory_data_register; // REALCONS extension in scp.c
        extern int realcons_console_halt; // 1: CPU halted by realcons console
        extern volatile t_bool sim_is_running; // global in scp.c

        // REALCONS extension in pdp18b_cpu.c
        extern int32 PC_Global;
        extern int32 LAC;
        extern int32 MQ;
        extern int32 SC;
        extern int32 XR;
        extern int32 LR;
        extern int32 SR;
        extern int32 ASW;
        extern int32 realcons_IR;
        extern int32 realcons_operand_address_register;
        extern int32 memm; // memory mode. 1 = BANK MODE, 0 = PAGE MODE with indexing
        extern int32 memm_init; // memory_mode after reset
        extern int32 api_enb; /* API enable */
        extern int32 api_act; /* API active */
        extern int32 ion;

        realcons_console_halt = 0;
        // is "sim_is_running" indeed identical with our "cpu_is_running" ?
        // may cpu stops, but some device are still serviced?
        _this->cpusignal_run = &sim_is_running;

        // from scp.c
        _this->cpusignal_memory_address_phys_register = &realcons_memory_address_phys_register;
        _this->cpusignal_register_name = &realcons_register_name; // pseudo: name of last accessed register
        _this->cpusignal_memory_data_register = &realcons_memory_data_register;
        _this->cpusignal_console_halt = &realcons_console_halt;

        _this->cpusignal_pc = &PC_Global;
        _this->cpusignal_lac = &LAC;
        _this->cpusignal_ir = &realcons_IR;
        _this->cpusignal_mq = &MQ;
        _this->cpusignal_sc = &SC;
        _this->cpusignal_xr = &XR;
        _this->cpusignal_lr = &LR;
        _this->cpusignal_switch_register = &SR;
        _this->cpusignal_address_switches = &ASW;
        _this->cpusignal_bank_mode = &memm;
        _this->cpusignal_bank_mode_init = &memm_init;

        _this->cpusignal_api_enable = &api_enb;
        _this->cpusignal_api_active = &api_act;
        _this->cpusignal_pi_enable = &ion;

        _this->cpusignal_operand_address_register = &realcons_operand_address_register;

#ifdef TODO
        // from pdp8_cpu.c
        _this->cpusignal_switch_register = &OSR;// see pdp8_cpu.c
        _this->cpusignal_if_pc = &saved_PC;// IF'PC, 15bit PC
        _this->cpusignal_df = &saved_DF;
        _this->cpusignal_link_accumulator = &saved_LAC;
        _this->cpusignal_multiplier_quotient = &saved_MQ;

        _this->cpusignal_instruction_decode = &realcons_instruction_decode;

        // bit mask for decoded instruction, see REALCONS_pdp15_INSTRUCTION_DECODE_*

        //
        _this->cpusignal_majorstate_curinstr = &realcons_majorstate_curinstr;
        _this->cpusignal_majorstate_last = &realcons_majorstate_last;
        _this->cpusignal_flipflops = &realcons_flipflops;
#endif
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
                (console_controller_event_func_t) realcons_console_pdp15__event_run_start;
        realcons_event_operator_halt = NULL;
        realcons_event_step_start =
                (console_controller_event_func_t) realcons_console_pdp15__event_step_start;
        realcons_event_step_halt =
                (console_controller_event_func_t) realcons_console_pdp15__event_operator_step_halt;
        realcons_event_operator_exam =
                realcons_event_operator_deposit =
                        (console_controller_event_func_t) realcons_console_pdp15__event_operator_exam_deposit;
		realcons_event_cpu_reset = NULL ;
        realcons_event_operator_reg_exam = realcons_event_operator_reg_deposit = NULL;
    }
#ifdef TODO
    {
        // set event ptrs in pdp8_cpu.c
        extern console_controller_event_func_t realcons_event_opcode_halt;
        //			_this->realcons->events_cpu_generic.opcode_any =
        //				(console_controller_event_func_t)realcons_console_pdp15__event_opcode_any;
        realcons_event_opcode_halt = (console_controller_event_func_t)realcons_console_pdp15__event_opcode_halt;
    }
#endif
}

// setup first state
t_stat realcons_console_pdp15_reset(realcons_console_logic_pdp15_t *_this)
{
    _this->realcons->simh_cmd_buffer[0] = '\0';
    // The 8i console has no addr register on its own, the CPU memory_address_phys_register is used.

    /*
     * direct links to all required controls.
     * See PiDP8 Blinkenlight API server project. Control names exactly like printed onto the PDP15 acryl
     */
// *** Indicator Board ***
// Comments: schematic indicator component number & name
    if (!(_this->lamp_dch_active = realcons_console_get_output_control(_this->realcons,
            "DCH_ACTIVE")))
        return SCPE_NOATT;
    if (!(_this->lamps_api_states_active = realcons_console_get_output_control(_this->realcons,
            "API_STATES_ACTIVE")))
        return SCPE_NOATT;
    if (!(_this->lamp_api_enable = realcons_console_get_output_control(_this->realcons,
            "API_ENABLE")))
        return SCPE_NOATT;
    if (!(_this->lamp_pi_active = realcons_console_get_output_control(_this->realcons, "PI_ACTIVE")))
        return SCPE_NOATT;
    if (!(_this->lamp_pi_enable = realcons_console_get_output_control(_this->realcons, "PI_ENABLE")))
        return SCPE_NOATT;
    if (!(_this->lamp_mode_index = realcons_console_get_output_control(_this->realcons,
            "MODE_INDEX")))
        return SCPE_NOATT;
    if (!(_this->lamp_major_state_fetch = realcons_console_get_output_control(_this->realcons,
            "STATE_FETCH")))
        return SCPE_NOATT;
    if (!(_this->lamp_major_state_inc = realcons_console_get_output_control(_this->realcons,
            "STATE_INC")))
        return SCPE_NOATT;
    if (!(_this->lamp_major_state_defer = realcons_console_get_output_control(_this->realcons,
            "STATE_DEFER")))
        return SCPE_NOATT;
    if (!(_this->lamp_major_state_eae = realcons_console_get_output_control(_this->realcons,
            "STATE_EAE")))
        return SCPE_NOATT;
    if (!(_this->lamp_major_state_exec = realcons_console_get_output_control(_this->realcons,
            "STATE_EXEC")))
        return SCPE_NOATT;
    if (!(_this->lamps_time_states = realcons_console_get_output_control(_this->realcons,
            "TIME_STATES")))
        return SCPE_NOATT;

    if (!(_this->lamp_extd = realcons_console_get_output_control(_this->realcons, "EXTD")))
        return SCPE_NOATT;
    if (!(_this->lamp_clock = realcons_console_get_output_control(_this->realcons, "CLOCK")))
        return SCPE_NOATT;
    if (!(_this->lamp_error = realcons_console_get_output_control(_this->realcons, "ERROR")))
        return SCPE_NOATT;
    if (!(_this->lamp_prot = realcons_console_get_output_control(_this->realcons, "PROT")))
        return SCPE_NOATT;
    if (!(_this->lamp_link = realcons_console_get_output_control(_this->realcons, "LINK")))
        return SCPE_NOATT;
    if (!(_this->lamp_register = realcons_console_get_output_control(_this->realcons, "REGISTER")))
        return SCPE_NOATT;

    if (!(_this->lamp_power = realcons_console_get_output_control(_this->realcons, "POWER")))
        return SCPE_NOATT;
    if (!(_this->lamp_run = realcons_console_get_output_control(_this->realcons, "RUN")))
        return SCPE_NOATT;
    if (!(_this->lamps_instruction = realcons_console_get_output_control(_this->realcons,
            "INSTRUCTION")))
        return SCPE_NOATT;
    if (!(_this->lamp_instruction_defer = realcons_console_get_output_control(_this->realcons,
            "INSTRUCTION_DEFER")))
        return SCPE_NOATT;
    if (!(_this->lamp_instruction_index = realcons_console_get_output_control(_this->realcons,
            "INSTRUCTION_INDEX")))
        return SCPE_NOATT;
    if (!(_this->lamp_memory_buffer = realcons_console_get_output_control(_this->realcons,
            "MEMORY_BUFFER")))
        return SCPE_NOATT;

// *** Switch Board ***
    if (!(_this->switch_stop = realcons_console_get_input_control(_this->realcons, "STOP")))
        return SCPE_NOATT;
    if (!(_this->switch_reset = realcons_console_get_input_control(_this->realcons, "RESET")))
        return SCPE_NOATT;
    if (!(_this->switch_read_in = realcons_console_get_input_control(_this->realcons, "READ_IN")))
        return SCPE_NOATT;
    if (!(_this->switch_reg_group = realcons_console_get_input_control(_this->realcons, "REG_GROUP")))
        return SCPE_NOATT;
    if (!(_this->switch_clock = realcons_console_get_input_control(_this->realcons, "CLOCK")))
        return SCPE_NOATT;
    if (!(_this->switch_bank_mode = realcons_console_get_input_control(_this->realcons, "BANK_MODE")))
        return SCPE_NOATT;
    if (!(_this->switch_rept = realcons_console_get_input_control(_this->realcons, "REPT")))
        return SCPE_NOATT;
    if (!(_this->switch_prot = realcons_console_get_input_control(_this->realcons, "PROT")))
        return SCPE_NOATT;
    if (!(_this->switch_sing_time = realcons_console_get_input_control(_this->realcons, "SING_TIME")))
        return SCPE_NOATT;
    if (!(_this->switch_sing_step = realcons_console_get_input_control(_this->realcons, "SING_STEP")))
        return SCPE_NOATT;
    if (!(_this->switch_sing_inst = realcons_console_get_input_control(_this->realcons, "SING_INST")))
        return SCPE_NOATT;
    if (!(_this->switch_address = realcons_console_get_input_control(_this->realcons, "ADDRESS")))
        return SCPE_NOATT;

    if (!(_this->switch_start = realcons_console_get_input_control(_this->realcons, "START")))
        return SCPE_NOATT;
    if (!(_this->switch_exec = realcons_console_get_input_control(_this->realcons, "EXECUTE")))
        return SCPE_NOATT;
    if (!(_this->switch_cont = realcons_console_get_input_control(_this->realcons, "CONT")))
        return SCPE_NOATT;
    if (!(_this->switch_deposit_this = realcons_console_get_input_control(_this->realcons,
            "DEPOSIT_THIS")))
        return SCPE_NOATT;
    if (!(_this->switch_examine_this = realcons_console_get_input_control(_this->realcons,
            "EXAMINE_THIS")))
        return SCPE_NOATT;
    if (!(_this->switch_deposit_next = realcons_console_get_input_control(_this->realcons,
            "DEPOSIT_NEXT")))
        return SCPE_NOATT;
    if (!(_this->switch_examine_next = realcons_console_get_input_control(_this->realcons,
            "EXAMINE_NEXT")))
        return SCPE_NOATT;
    if (!(_this->switch_data = realcons_console_get_input_control(_this->realcons, "DATA")))
        return SCPE_NOATT;

    if (!(_this->switch_power = realcons_console_get_input_control(_this->realcons, "POWER")))
        return SCPE_NOATT;
    if (!(_this->potentiometer_repeat_rate = realcons_console_get_input_control(_this->realcons,
            "REPEAT_RATE")))
        return SCPE_NOATT;

    if (!(_this->switch_register_select = realcons_console_get_input_control(_this->realcons,
            "REGISTER_SELECT")))
        return SCPE_NOATT;

    // verify: is the power switch turned ON?

    _this->last_repeat_rate_event_msec = 0;

    return SCPE_OK;
}

// processes the REPEAT RATe stuff and momentary action switches
static int momentary_switch_signal(realcons_console_logic_pdp15_t *_this,
        blinkenlight_control_t *action_switch, int repeat_triggered)
{
    int result = 0;
    int is_repeatable;

    // is the switch repeatable?
    is_repeatable = (action_switch == _this->switch_start || action_switch == _this->switch_exec
            || action_switch == _this->switch_cont || action_switch == _this->switch_examine_this
            || action_switch == _this->switch_examine_next
            || action_switch == _this->switch_deposit_this
            || action_switch == _this->switch_deposit_next);

    // switch signal on first press
    if (action_switch->value && !action_switch->value_previous) {
        if (_this->realcons->debug)
            printf("Switch %s manual action detected", action_switch->name);
        result = 1;
    }
    // switch signal on repeat rate, if switch pressed
    if (is_repeatable && repeat_triggered && action_switch->value) {
        if (_this->realcons->debug)
            printf("Switch %s repeated action detected", action_switch->name);
        result = 1;
    }

    action_switch->value_previous = action_switch->value; // clear trigger condition
    return result;
}

// process panel state.
// operates on Blinkenlight_API panel structs,
// but RPC communication is done external
t_stat realcons_console_pdp15_service(realcons_console_logic_pdp15_t *_this)
{
    blinkenlight_panel_t *p = _this->realcons->console_model; // alias

    int console_mode;
    int repeat_triggered;
    int single;

    /* test time expired? */

    // STOP by activating HALT?
    if (_this->switch_stop->value && !SIGNAL_GET(cpusignal_console_halt)) {
        // must be done by SimH.pdp15_cpu.c!
    }

    // CONSOLE: processor accepts commands from console panel when HALTed
    console_mode = !SIGNAL_GET(cpusignal_run) || SIGNAL_GET(cpusignal_console_halt);

    /*************************************************************
     * eval switches
     * react on changed action switches
     */

    // always set switch register
    SIGNAL_SET(cpusignal_switch_register, _this->switch_data->value);
    SIGNAL_SET(cpusignal_address_switches, _this->switch_address->value);

    // fetch switch register
    //SIGNAL_SET(cpusignal_switch_register, _this->switch_switch_register->value);
    if (_this->switch_power->value == 0 && _this->switch_power->value_previous == 1) {
        // Power switch transition to POWER OFF: terminate SimH
        // This is drastic, but will teach users not to twiddle with the power switch.
        // when panel is disconnected, panel mode goes to POWERLESS and power switch goes OFF.
        // But shutdown sequence is not initiated, because we're disconnected then.

        SIGNAL_SET(cpusignal_console_halt, 1); // stop execution
        realcons_simh_add_cmd(_this->realcons, "quit"); // do not confirm the quit with ENTER
        return SCPE_OK;
    }

    // fetch HALT mode, must be sensed by simulated processor to produce state OPERATOR_HALT
    SIGNAL_SET(cpusignal_console_halt, (int ) _this->switch_stop->value);




    /***************************************************
     * eval switches

     */
    /* "STOP is the only switch active while the machine is running. If, at any time,
     * the machine must be reset while the RUN light is on, the RESET and STOP switches
     * should be pressed simultaneously. This is an unconditional reset procedure that
     *should be used with caution, beacuse data can be lost."*/
    if (console_mode) {

        // "With REPEAT switch in the ON position, the processor will repeat the key function
        //    depressed by the operator at the rate specified by the repeat clock.
        //    Start - program execution will restart at the repeat speed after the machine halts.
        //    Execute - the instruction in the data switches will be executed at the repeat
        //    clock rate.
        //    Continue - program execution will continue at the repeat speed after halting.
        //    Deposit: This, Next.Examine : This, Next - the Deposit, Deposit Next or Examine
        //    Next function will be repeated.
        //    Depressing STOP or turning off the repeat switch will halt the repeat action."
        //      (this console has no STOP LOOP button)
        repeat_triggered = 0;
        if (_this->switch_rept->value) {
            int repeat_intervall_msec;
            int repeat_rate_hz;
            // process repeat rate first
            // "A variable-pot/switch provides the POWER ON/ OFF control at one end of its
            // rotation and variable repeat speed(approximately 1 Hz to 10 kHz) over the
            // remainder of its rotation."
#define REPEAT_FMAX 200         // 0..255 => 0..FMAX. Originally FMAX=10kHz, too fast for simulation
            repeat_rate_hz = (REPEAT_FMAX * (int)_this->potentiometer_repeat_rate->value) / 255;
            if (repeat_rate_hz == 0)
                repeat_intervall_msec = INT_MAX;
            else
                repeat_intervall_msec = 1000 / repeat_rate_hz;

            // repeat rate running, trigger now?
            if (_this->realcons->service_cur_time_msec
                > _this->last_repeat_rate_event_msec + repeat_intervall_msec) {
                _this->last_repeat_rate_event_msec = _this->realcons->service_cur_time_msec;
                repeat_triggered = 1; // trigger immediately on action of REPT switch
                // if REPT switch is is going OFF->ON , last_repeat_rate_event_msec is very old and
                // a trigger occurs immediately.
            }
        }

        // "Examine: Places the contents of the memory location specified by the address switches
        // into the memory buffer register. The address is loaded into the OA(operand
        //  address) register.
        // Examine Next:  Places the contents of the memory location specified by the OA + 1 (operand
        // address plus 1) into the memory buffer register. Sequential memory locations may
        // be examined using the Examine - Next switch.
        if (momentary_switch_signal(_this, _this->switch_examine_this, repeat_triggered)) {
            unsigned addrval;
            addrval = (realcons_machine_word_t)_this->switch_address->value;
            addrval &= 077777; // 15 bit addr
            SIGNAL_SET(cpusignal_operand_address_register, addrval);
            // generate simh "exam cmd". Always octal, should use SimH-radix
            realcons_simh_add_cmd(_this->realcons, "examine %o\n", addrval);
        }
        if (momentary_switch_signal(_this, _this->switch_examine_next, repeat_triggered)) {
            unsigned addrval;
            addrval = SIGNAL_GET(cpusignal_operand_address_register) + 1;
            addrval &= 077777; // 15 bit addr
            SIGNAL_SET(cpusignal_operand_address_register, addrval);
            // generate simh "exam cmd". Always octal, should use SimH-radix
            realcons_simh_add_cmd(_this->realcons, "examine %o\n", addrval);
        }

        // "Deposit: Deposits the contents of the data switches into the memory location specified by
        // the address switches. After the transfer, the memory locations address is in the
        // OA (operand address) register and the contents of the accumulator switches are
        // in the MO (memory out) register.
        // Deposit Next: Deposits the contents of the data switches into the location given by OA +/- 1.
        // This permits the loading of sequential memory locations without the need of
        // loading the address each time."
        // (The  "+/-" in the docs is interpreted as typo, only "+" is implemented.)
        if (momentary_switch_signal(_this, _this->switch_deposit_this, repeat_triggered)) {
            unsigned addrval, dataval;
            addrval = (realcons_machine_word_t)_this->switch_address->value;
            addrval &= 077777; // 15 bit addr
            SIGNAL_SET(cpusignal_operand_address_register, addrval);
            dataval = (realcons_machine_word_t)_this->switch_data->value;
            // produce SimH cmd. Always octal, should use SimH-radix
            realcons_simh_add_cmd(_this->realcons, "deposit %o %o\n", addrval, dataval);
        }
        if (momentary_switch_signal(_this, _this->switch_deposit_next, repeat_triggered)) {
            unsigned addrval, dataval;
            addrval = SIGNAL_GET(cpusignal_operand_address_register) + 1;
            addrval &= 077777; // 15 bit addr
            SIGNAL_SET(cpusignal_operand_address_register, addrval);
            dataval = (realcons_machine_word_t)_this->switch_data->value;
            // produce SimH cmd. Always octal, should use SimH-radix
            realcons_simh_add_cmd(_this->realcons, "deposit %o %o\n", addrval, dataval);
        }

        // RUN control

        single = _this->switch_sing_inst->value || _this->switch_sing_step->value
            || _this->switch_sing_time->value;

        if (momentary_switch_signal(_this, _this->switch_reset, repeat_triggered)) {
            SIGNAL_SET(cpusignal_console_halt, 1); // stop execution
            realcons_simh_add_cmd(_this->realcons, "reset all\n");
        }
        if (console_mode && momentary_switch_signal(_this, _this->switch_start, repeat_triggered)) {
            unsigned addrval = (unsigned)_this->switch_address->value;
            // "Initiates program execution at the location specified by the address switches."
            if (single) {
                realcons_simh_add_cmd(_this->realcons, "deposit pc %o\n", addrval);
                realcons_simh_add_cmd(_this->realcons, "step\n");
            }
            else
                realcons_simh_add_cmd(_this->realcons, "run %o\n", addrval);
        }
        if (console_mode && momentary_switch_signal(_this, _this->switch_cont, repeat_triggered)) {
            // "Resumes program execution from the location specified by the program counter
            // (PC).Also used to advance machine while in single time, step or instruction."
            if (single)
                realcons_simh_add_cmd(_this->realcons, "step\n");
            else
                realcons_simh_add_cmd(_this->realcons, "continue\n");
        }
        if (console_mode && momentary_switch_signal(_this, _this->switch_exec, repeat_triggered)) {
            // "Executes the instruction in the data switches and halts after execution."
            sprintf(_this->realcons->simh_cmd_buffer, "; EXEC switch not implemented\n");
        }
        if (console_mode && momentary_switch_signal(_this, _this->switch_read_in, repeat_triggered)) {
            char *boot_image_filepath = _this->realcons->boot_image_filepath;
            // unsigned addrval = _this->switch_address->value;
            // "Initiates the read-in of paper tape punched in binary code (each set of three
            // 6 - bit lines read from tape forms one 18 - bit computer word).Storage of words
            // read - in begins at the memory location specified by the ADDRESS switches. At the
            // completion of tape read - in, the processor reads the last word entered and
            // executes it."
            // SIGNAL_SET(cpusignal_console_halt, 1); // stop execution
            // Here SimH load, see pdp18b.doc, "LOAD" command
            if (strlen(boot_image_filepath) == 0)
                realcons_simh_add_cmd(_this->realcons,
                    "; boot image file not set, use \"set realcons bootimage=<filepath>\"\n");
            else {
                realcons_simh_add_cmd(_this->realcons, "load %s\n", boot_image_filepath);
                // PC is already set
                realcons_simh_add_cmd(_this->realcons, "run\n");
            }
        }

        // BANK MODE:
        /* "When set (back half of switch pressed), pressing START causes the system to
         * start in Bank mode permitting direct addressing of 8, 192 (17777o) word of core
         * memory. When switch is not set (front half pressed), pressing START causes the
         * system to start in Page mode permitting direct addressing of 4,096 (7777o) words
         * of core memory".
         * BANK mode set by EBA/DBA instruction, and after Reset through memm_init
         * so interpret the BANK MODE switch as Reset-default
         * Bank mode  = PDP-9 compatibility
         */
        if (_this->switch_bank_mode->value)
            SIGNAL_SET(cpusignal_bank_mode_init, 1);
        else
            SIGNAL_SET(cpusignal_bank_mode_init, 0);
    } // if(console_mode)

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
        unsigned regval; // value of 18 REGISTER lamps + LINK lamp

        // power always oN
        _this->lamp_power->value = 1;

        // RUN
        _this->lamp_run->value = !console_mode;

        // DCH = Data CHannel Facility

        // Interrupts
        // states active: bit mask encoding
        _this->lamps_api_states_active->value = SIGNAL_GET(cpusignal_api_active) & 0377;

        _this->lamp_api_enable->value = !!SIGNAL_GET(cpusignal_api_enable);

        _this->lamp_pi_enable->value = !!SIGNAL_GET(cpusignal_pi_enable);

        // no signal "PI_ACTIVE in SimH? PI active = API LEVEL 3 = bit 0020?
        if (SIGNAL_GET(cpusignal_pi_enable) && (SIGNAL_GET(cpusignal_api_active) & 0020))
            _this->lamp_pi_active->value = 1;
        else
            _this->lamp_pi_active->value = 0;

        // MEMORY BUFFER
        _this->lamp_memory_buffer->value = SIGNAL_GET(cpusignal_memory_data_register);

        // there are various opcode formats, see pdp18b_cpu.c
        // for indirect/index, see PDP-15 Systems Reference Manual (Aug 1969, DEC-15-BRZA-D).pdf , page 6-3ff
        // BANK moder = addresses are 13 bits = 8Kwords
        // page mode = addresses are 12 bits = 4Kwords + 1 "index" bit
        //      if index is set, XR register is added to address
        {
            unsigned ir = SIGNAL_GET(cpusignal_ir);
            unsigned ir_opcode = (ir >> 14) & 017; // upper 4 bits
            unsigned ir_indirect_defer = (ir >> 13) & 1; // in all formats bit 4
            unsigned ir_index = 0;
            unsigned bank_mode = SIGNAL_GET(cpusignal_bank_mode); // 0 = page mode, 1 = bank mode, switched by EBA/DBA

            switch (ir_opcode) {
            case 015: // EAE
                ir_indirect_defer = 0;
                break;
            case 016: // IO transfer, PDP-15 floating point, PDP-15 index operate
                // index valid
                break;
            case 017: // operate, load immediate
                ir_indirect_defer = 0;
                break;
            default:
                // "The PDP-15 in page mode trades an address bit for indexing capability:"
                if (!bank_mode)
                    ir_index = (ir >> 12) & 1; // bit 5  = address MSB
            }
            _this->lamps_instruction->value = ir_opcode;
            _this->lamp_instruction_defer->value = ir_indirect_defer;
            _this->lamp_instruction_index->value = ir_index;
            _this->lamp_mode_index->value = !bank_mode;
        }

        regval = 0; // default for "not implemented"
        if (_this->switch_reg_group->value == 0) {
            // "The 12 position Register Select rotary switch can select the following registers
            // for viewing in the REGISTER indicators(under control of the Register Group switch).
            // Register Group Switch OFF :
            switch (_this->switch_register_select->value) {
            case 04000: // AC      Accumulator + LINK
                regval = SIGNAL_GET(cpusignal_lac);
                break;
            case 02000: // PC      Program Counter
                regval = SIGNAL_GET(cpusignal_pc);
                break;
            case 01000: // OA      Operand Address
                regval = SIGNAL_GET(cpusignal_operand_address_register);
                break;
            case 00400: // MQ      Multiplexer Quotient
                regval = SIGNAL_GET(cpusignal_mq);
                break;
            case 00200: // L, SC   Priority Level / Step Counter
                regval = SIGNAL_GET(cpusignal_sc);
                // priority level ???
                break;
            case 00100: // XR      Index Register
                regval = SIGNAL_GET(cpusignal_xr);
                break;
            case 00040: // LR      Limit Register
                regval = SIGNAL_GET(cpusignal_lr);
                break;
            case 00020: // EAE
                // "DEC-15-H2BB-D_MaintManVol1.pdf", page 6-22
                // "The EAE switch position indicates the complement of 18 control fucntions"
                // => 18 hardware signal, none is emulated
                break;
            case 00010: // DSR     Data Storage Register
                break;
            case 00004: // IOB     I / O Bus
                break;
            case 00002: // STA     I / O Status
                break;
            case 00001: // MO      Memory Out
                regval = SIGNAL_GET(cpusignal_memory_data_register);
                break;
            }
        } else {
            // Register Group Switch ON :
            switch (_this->switch_register_select->value) {
            case 04000: // A BU    A Bus
                break;
            case 02000: // B BU    B Bus
                break;
            case 01000: // C BU    C Bus
                break;
            case 00400: // SFT     Shift Bus
                break;
            case 00200: // IOA     I / O Address
                break;
            case 00100: // SUM     Sum Bus
                break;
            case 00040: // MI      Maintenance I
                break;
            case 00020: // M2      Maintenance II
                break;
            case 00010: // MDL     Memory Data Lines
                regval = SIGNAL_GET(cpusignal_memory_data_register); // better than nothing
                break;
            case 00004: // MMA     Memory Address
                regval = SIGNAL_GET(cpusignal_memory_address_phys_register);
                break;
            case 00002: // MMB     Memory Buffer
                regval = SIGNAL_GET(cpusignal_memory_data_register);
                break;
            case 00001: // MST     Memory Status
                break;
            }
        }
        _this->lamp_register->value = regval & DATA18BITMASK;
        _this->lamp_link->value = !!(regval & ~DATA18BITMASK); // ON, if more than 18 bits set

        // if CPU is running, simulate some plausible light patterns (SimH does not simulate single cycles)
        // FETCH 50%, EXEC 33%, DEFER 20%, INC 20%, EAE%: 0 or 10%
        // Time states: assume every cycle as 3 time states: all 33%
        if (console_mode) {
            // all OFF?
            _this->lamp_major_state_fetch->value = 0;
            _this->lamp_major_state_inc->value = 0;
            _this->lamp_major_state_defer->value = 0;
            _this->lamp_major_state_eae->value = 0;
            _this->lamp_major_state_exec->value = 0;
            _this->lamps_time_states->value = 0;
        } else {
            extern int32 realcons_eae_enabled;
            // running
            _this->lamp_major_state_fetch->value = (_this->realcons->service_cycle_count % 2 == 0);
            _this->lamp_major_state_inc->value = (_this->realcons->service_cycle_count % 5 == 0);
            _this->lamp_major_state_defer->value = (_this->realcons->service_cycle_count % 5 == 0);
            if (realcons_eae_enabled)
                _this->lamp_major_state_eae->value =
                        (_this->realcons->service_cycle_count % 2 == 0);
            else
                _this->lamp_major_state_eae->value = 0;
            _this->lamp_major_state_exec->value = (_this->realcons->service_cycle_count % 2 == 0);
            switch (_this->realcons->service_cycle_count % 3) {
            case 0:
                _this->lamps_time_states->value = 1;
                break;
            case 1:
                _this->lamps_time_states->value = 2;
                break;
            case 2:
                _this->lamps_time_states->value = 4;
                break;
            }
        }

        /* dummies
         */
        _this->lamp_prot->value = _this->switch_prot->value;
        _this->lamp_clock->value = _this->switch_clock->value;

    } // if ! lamptest

    return SCPE_OK;
}

/*
 * start 1 sec test sequence.
 * - lamps ON for 1 sec
 * - print state of all switches
 */
int realcons_console_pdp15_test(realcons_console_logic_pdp15_t *_this, int arg)
{
    // send end time for test: 1 second = curtime + 1000
    // lamp test is set in service()
    _this->realcons->timer_running_msec[TIMER_TEST] = _this->realcons->service_cur_time_msec
            + TIME_TEST_MS;

    realcons_printf(_this->realcons, stdout, "Verify the lamp test!\n");
    realcons_printf(_this->realcons, stdout, "Switch 'POWER ON/OFF' ....... = %llo\n",
            _this->switch_power->value);
    realcons_printf(_this->realcons, stdout, "Switch 'STOP' ............... = %llo\n",
            _this->switch_stop->value);
    realcons_printf(_this->realcons, stdout, "Switch 'RESET' .............. = %llo\n",
            _this->switch_reset->value);
    realcons_printf(_this->realcons, stdout, "Switch 'READ IN' ............ = %llo\n",
            _this->switch_read_in->value);
    realcons_printf(_this->realcons, stdout, "Switch 'REG GROUP' .......... = %llo\n",
            _this->switch_reg_group->value);
    realcons_printf(_this->realcons, stdout, "Switch 'CLOCK' .............. = %llo\n",
            _this->switch_clock->value);
    realcons_printf(_this->realcons, stdout, "Switch 'BANK MODE' .......... = %llo\n",
            _this->switch_bank_mode->value);
    realcons_printf(_this->realcons, stdout, "Switch 'REPT' ............... = %llo\n",
            _this->switch_rept->value);
    realcons_printf(_this->realcons, stdout, "Switch 'PROT' ............... = %llo\n",
            _this->switch_prot->value);
    realcons_printf(_this->realcons, stdout, "Switch 'SING TIME' .......... = %llo\n",
            _this->switch_sing_time->value);
    realcons_printf(_this->realcons, stdout, "Switch 'SING STEP' .......... = %llo\n",
            _this->switch_sing_step->value);
    realcons_printf(_this->realcons, stdout, "Switch 'SING INST' .......... = %llo\n",
            _this->switch_sing_inst->value);
    realcons_printf(_this->realcons, stdout, "Switches 'ADDRESS' .......... = %llo\n",
            _this->switch_address->value);
    realcons_printf(_this->realcons, stdout, "Switch 'START' .............. = %llo\n",
            _this->switch_start->value);
    realcons_printf(_this->realcons, stdout, "Switch 'EXEC' ............... = %llo\n",
            _this->switch_exec->value);
    realcons_printf(_this->realcons, stdout, "Switch 'CONT' ............... = %llo\n",
            _this->switch_cont->value);
    realcons_printf(_this->realcons, stdout, "Switch 'DEPOSIT THIS' ....... = %llo\n",
            _this->switch_deposit_this->value);
    realcons_printf(_this->realcons, stdout, "Switch 'DEPOSIT NEXT' ....... = %llo\n",
            _this->switch_deposit_next->value);
    realcons_printf(_this->realcons, stdout, "Switch 'EXAMINE THIS' ....... = %llo\n",
            _this->switch_examine_this->value);
    realcons_printf(_this->realcons, stdout, "Switch 'EXAMINE NEXT' ....... = %llo\n",
            _this->switch_examine_next->value);
    realcons_printf(_this->realcons, stdout, "Switches 'DATA' ............. = %llo\n",
            _this->switch_data->value);
    realcons_printf(_this->realcons, stdout, "Potentiometer 'REPEAT RATE'   = %llo\n",
            _this->potentiometer_repeat_rate->value);

    return 0; // OK
}
