/* realcons_pdp15.h: Logic for the PDP-15 panel

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


   18-Jun-2016  JH      created
*/

#ifndef REALCONS_CONSOLE_PDP15_H_
#define REALCONS_CONSOLE_PDP15_H_

#include "realcons.h"




typedef struct
{
    /* base class members, identical for all panels */
    struct realcons_struct *realcons; // parent
    // parant is used to link to other data structs:
    //  example: Blinkenlight API panel struct = realcons->blinkenlight_panel

    /* Signals for SimH's generic console interface
     * (halt, start, singlestep, exam deposit).
     * Signals in VHDL are variables holding values of physical wires
     * Signals in REALCONS are pointers to variables either in the SimH simulation code, or
     * in panel logic. They connect simulation and panel. A certain signal is can be unidirectional
     * (either set by simulator and read by panel logic, ore vice verse) or be bidirectional.
     */
    t_addr *cpusignal_memory_address_phys_register; // address of last bus cycle (EXAM/DEPOSIT)
    char **cpusignal_register_name; // name of last accessed exam/deposit register
    t_value *cpusignal_memory_data_register; // data of last bus cycle
    int *cpusignal_console_halt; // 1, if a real console halts program execution
    volatile t_bool *cpusignal_run; // 1, if simulated cpu is running

    /* Signals for SimH's PDP15 cpu. */
    int32   *cpusignal_lac; // Link and Accumulator.
    int32   *cpusignal_pc;
    int32   *cpusignal_ir;
    int32   *cpusignal_mq; // multiplier - quotient
    int32   *cpusignal_sc; // EAE step counter
    int32   *cpusignal_xr; // index register
    int32   *cpusignal_lr; // limit register
    int32   *cpusignal_switch_register; 
    int32   *cpusignal_address_switches;
    int32   *cpusignal_bank_mode; // 1 = bank mode, 0 = page mode, switched by EBA/DBA
    int32   *cpusignal_bank_mode_init; // bank mode after RESET
                                  // bank-mode = comaptibility with PDP-9

    int32   *cpusignal_api_enable; /* API enable */
    int32   *cpusignal_api_active; /* API active */

                                   //   int32 api_req = 0;                                      /* API requests */
    int32   *cpusignal_pi_enable; // Program interrupt ENABLE/DISABLE by opcodes IOF/ION

    int32 	*cpusignal_multiplier_quotient ;
    int32   *cpusignal_operand_address_register; // effektive address, console helper

    // see  ...\07.3_blinkenlight_server_pdp15
// *** Indicator Board ***
// Comments: schematic indicator component number & name
    blinkenlight_control_t *lamp_dch_active ; // I1  "DCH ACTIVE"
    blinkenlight_control_t *lamps_api_states_active; // I2-I6,I8-I10 "PL00-07"
    blinkenlight_control_t *lamp_api_enable; // I11 "API ENABLE"
    blinkenlight_control_t *lamp_pi_active; // I12 "PI ACTIVE"
    blinkenlight_control_t *lamp_pi_enable ; // I13 "PI ENABLE"
    blinkenlight_control_t *lamp_mode_index ; // I14 "INDEX MODE"
    blinkenlight_control_t *lamp_major_state_fetch ; // I15 "FETCH"
    blinkenlight_control_t *lamp_major_state_inc ; // I16 "INC"
    blinkenlight_control_t *lamp_major_state_defer ; // I17 "DEFER"
    blinkenlight_control_t *lamp_major_state_eae ; // I18 "EAE"
    blinkenlight_control_t *lamp_major_state_exec ; // I19 "EXECUTE"
    blinkenlight_control_t *lamps_time_states ; // I20-I22 "TS1-TS3"

    blinkenlight_control_t *lamp_extd ; // I25 "EXTD"
    blinkenlight_control_t *lamp_clock ; // I26 "CLOCK"
    blinkenlight_control_t *lamp_error ; // I27 "ERROR"
    blinkenlight_control_t *lamp_prot ; // I28 "PRTCT"  "protect"
    blinkenlight_control_t *lamp_link ; // I7 "LINK"
    blinkenlight_control_t *lamp_register ; // I29-I46 "R00-R17"

    blinkenlight_control_t *lamp_power ;  // I24 "POWER"
    blinkenlight_control_t *lamp_run ; // I23 "RUN"
    blinkenlight_control_t *lamps_instruction ; // I47-I50 "IR00-IR03"
    blinkenlight_control_t *lamp_instruction_defer ; // I51 "OP DEFER"
    blinkenlight_control_t *lamp_instruction_index ; // I52 "OP INDEX"
    blinkenlight_control_t *lamp_memory_buffer ; // I53-I70 "MB00..17"

// *** Switch Board ***
    blinkenlight_control_t *switch_stop; // S1 "STOP"
    blinkenlight_control_t *switch_reset ; // S2 "RESET"
    blinkenlight_control_t *switch_read_in ; // S3 "READ IN"
    blinkenlight_control_t *switch_reg_group ; // S4 "REG GROUP"
    blinkenlight_control_t *switch_clock ; // S5 "CLK"
    blinkenlight_control_t *switch_bank_mode ; // S6 "BANK MODE"
    blinkenlight_control_t *switch_rept ; // S7 "REPT"
    blinkenlight_control_t *switch_prot ; // S8 "PROT"
    blinkenlight_control_t *switch_sing_time ; // S9 "SING TIME"
    blinkenlight_control_t *switch_sing_step ; // S10 "SING STEP"
    blinkenlight_control_t *switch_sing_inst ; // S11 "SING INST"
    blinkenlight_control_t *switch_address ;  // S12-S26  "ADSW03-17"

    blinkenlight_control_t *switch_start ; // S27 "START"
    blinkenlight_control_t *switch_exec ; // S28 "EXECUTE"
    blinkenlight_control_t *switch_cont ; // S29 "CONT"
    blinkenlight_control_t *switch_deposit_this ; // S30 "DEP THIS"
    blinkenlight_control_t *switch_examine_this ; // S32 "EXAMINE THIS"
    blinkenlight_control_t *switch_deposit_next ; // constructed
    blinkenlight_control_t *switch_examine_next ; // constructed
    blinkenlight_control_t *switch_data ;     // S34-S51 "DSW0-17"

    blinkenlight_control_t *switch_power; // S53
    blinkenlight_control_t *potentiometer_repeat_rate;

    blinkenlight_control_t *switch_register_select ;  // S52
    // intern state registers of console panel
    unsigned run_state; // cpu can be: reset, halt, running.

    unsigned 	console_operand_address; // like address buffer on PDP-11's

    t_uint64 last_repeat_rate_event_msec; // future timestamp for next trigger of REPATEed actions

} realcons_console_logic_pdp15_t;

realcons_console_logic_pdp15_t *realcons_console_pdp15_constructor(
    struct realcons_struct *realcons);
void realcons_console_pdp15_destructor(realcons_console_logic_pdp15_t *_this);

struct realcons_console_controller_interface_struct;
void realcons_console_pdp15_interface_connect(realcons_console_logic_pdp15_t *_this,
        struct realcons_console_controller_interface_struct *intf, char *panel_name);

int realcons_console_pdp15_test(realcons_console_logic_pdp15_t *_this, int arg);

// process panel state. operates only "panel" data struct
t_stat realcons_console_pdp15_service(realcons_console_logic_pdp15_t *_this);
t_stat realcons_console_pdp15_reset(realcons_console_logic_pdp15_t *_this);

#endif /* REALCONS_CONSOLE_PDP15_H_ */
