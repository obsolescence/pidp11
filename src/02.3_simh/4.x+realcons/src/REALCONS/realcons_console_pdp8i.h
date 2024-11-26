/* realcons_pdp8i.h: Logic for the specific PDP8I panel

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


   29-Dec-2015  JH      created
*/

#ifndef REALCONS_CONSOLE_PDP8I_H_
#define REALCONS_CONSOLE_PDP8I_H_

#include "realcons.h"


// Signals set in the combinated instruction_decode word from cpu
#define REALCONS_PDP8I_INSTRUCTION_DECODE_AND	0x01
#define REALCONS_PDP8I_INSTRUCTION_DECODE_TAD	0x02
#define REALCONS_PDP8I_INSTRUCTION_DECODE_ISZ	0x04
#define REALCONS_PDP8I_INSTRUCTION_DECODE_DCA	0x08
#define REALCONS_PDP8I_INSTRUCTION_DECODE_JMS	0x10
#define REALCONS_PDP8I_INSTRUCTION_DECODE_JMP	0x20
#define REALCONS_PDP8I_INSTRUCTION_DECODE_IOT	0x40
#define REALCONS_PDP8I_INSTRUCTION_DECODE_OPR	0x80

// Signals set for the major state indicator of an instruction
// "One of these indicates that the processor is currently performing or
// has performed a certain cycle."
#define REALCONS_PDP8I_MAJORSTATE_FETCH			0x01
#define REALCONS_PDP8I_MAJORSTATE_EXECUTE		0x02
#define REALCONS_PDP8I_MAJORSTATE_DEFER			0x04
#define REALCONS_PDP8I_MAJORSTATE_WORDCOUNT		0x08
#define REALCONS_PDP8I_MAJORSTATE_CURRENTADDRESS 0x10
#define REALCONS_PDP8I_MAJORSTATE_BREAK			0x20

// Signals set by miscellaneous flip flops
// Ion indicates the 1 status of the INT.ENABLE flip - flop.
//		When lit, the interrupt control is enabled for
//		information exchange with an I / O device.
// Pause indicates the 1 status of the PAUSE flip - flop when lit.
//		The PAUSE flip - flop is set for 2.75 micro secs by any IOT
//		instruction that requires generation of IOP pulses or by
//		any EAE instruction that require shifting of information.
// Run	indicates the 1 status of the RUN flip - flop. When lit,
//      the internal timing circuits are enabled and the machine
//		performs instructions.
#define REALCONS_PDP8I_FLIPFLOP_ION				0x01
#define REALCONS_PDP8I_FLIPFLOP_PAUSE			0x02
#define REALCONS_PDP8I_FLIPFLOP_RUN				0x04 // not used, sim_is_running


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


	/* Signals for SimH's PDP8 cpu. */
	t_value *cpusignal_switch_register;
	int32 *cpusignal_if_pc; // IF'PC, 15bit PC. IF= instruction field
	int32	*cpusignal_df; // DF = data field
	int32 	*cpusignal_link_accumulator ;
	int32 	*cpusignal_multiplier_quotient ;
	int32 	*cpusignal_step_counter;


	int32	*cpusignal_instruction_decode; // bit mask for decoded isntruction, see REALCONS_PDP8I_INSTRUCTION_DECODE_*
	int32	*cpusignal_majorstate_curinstr; // sum of all states in curent instruction
	int32	*cpusignal_majorstate_last; // last state cpu was in (use after single step)

	int32	*cpusignal_flipflops; // see REALCONS_PDP8I_FLIPFLOPS_*

	// see  ...\projects\11pidp8\src\
    // input controls on the panel
    blinkenlight_control_t *keyswitch_power;
    blinkenlight_control_t *keyswitch_panel_lock;

	blinkenlight_control_t *switch_start;
	blinkenlight_control_t *switch_load_address;
	blinkenlight_control_t *switch_deposit;
	blinkenlight_control_t *switch_examine;
	blinkenlight_control_t *switch_continue;
	blinkenlight_control_t *switch_stop;
	blinkenlight_control_t *switch_single_step;
	blinkenlight_control_t *switch_single_instruction;
	blinkenlight_control_t *switch_switch_register;
	blinkenlight_control_t *switch_data_field;
	blinkenlight_control_t *switch_instruction_field;

	// output controls on the panel
	blinkenlight_control_t *led_program_counter;
	blinkenlight_control_t *led_inst_field;
	blinkenlight_control_t *led_data_field;
	blinkenlight_control_t *led_memory_address;
	blinkenlight_control_t *led_memory_buffer;
	blinkenlight_control_t *led_accumulator;
	blinkenlight_control_t *led_link;
	blinkenlight_control_t *led_multiplier_quotient;
	blinkenlight_control_t *led_and;
	blinkenlight_control_t *led_tad;
	blinkenlight_control_t *led_isz;
	blinkenlight_control_t *led_dca;
	blinkenlight_control_t *led_jms;
	blinkenlight_control_t *led_jmp;
	blinkenlight_control_t *led_iot;
	blinkenlight_control_t *led_opr;
	blinkenlight_control_t *led_fetch;
	blinkenlight_control_t *led_execute;
	blinkenlight_control_t *led_defer;
	blinkenlight_control_t *led_word_count;
	blinkenlight_control_t *led_current_address;
	blinkenlight_control_t *led_break;
	blinkenlight_control_t *led_ion;
	blinkenlight_control_t *led_pause;
	blinkenlight_control_t *led_run;
	blinkenlight_control_t *led_step_counter;

	// intern state registers of console panel
	unsigned run_state; // cpu can be: reset, halt, running.

} realcons_console_logic_pdp8i_t;

realcons_console_logic_pdp8i_t *realcons_console_pdp8i_constructor(
struct realcons_struct *realcons);
void realcons_console_pdp8i_destructor(realcons_console_logic_pdp8i_t *_this);

struct realcons_console_controller_interface_struct;
void realcons_console_pdp8i_interface_connect(realcons_console_logic_pdp8i_t *_this,
struct realcons_console_controller_interface_struct *intf, char *panel_name);

int realcons_console_pdp8i_test(realcons_console_logic_pdp8i_t *_this, int arg);

// process panel state. operates only "panel" data struct
t_stat realcons_console_pdp8i_service(realcons_console_logic_pdp8i_t *_this);
t_stat realcons_console_pdp8i_reset(realcons_console_logic_pdp8i_t *_this);

#endif /* REALCONS_CONSOLE_PDP8I_H_ */
