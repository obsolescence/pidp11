/* realcons_pdp11_70.h : Logic for the specific 11/70 panel

   Copyright (c) 2012-2016, Joerg Hoppe
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

   10-Mar-2018  JH      lot of fixes to match the tests at LCM "Miss Piggy"
      Dec-2015  JH      migration from SimH 3.82 to 4.x
                        CPU extensions moved to pdp11_cpu.c
   25-Mar-2012  JH      created
*/

#ifndef REALCONS_CONSOLE_PDP11_70_H_
#define REALCONS_CONSOLE_PDP11_70_H_

#include "realcons.h"

// CPU mode constants, only for information.
// Problem: cpu_mode constant are needed for "cm", but this include here
//      #include "pdp11_defs.h"
// damages the simulator,
// so define MD_* here identical to  pdp11_defs.h"

#define MD_KER          0
#define MD_SUP          1
#define MD_UND          2
#define MD_USR          3

// use mode codes for sim, because direct access to "cm"
#define REALCONS_CPU_PDP11_CPUMODE_PHYS	MD_UND		// MD_UND is no where used, Phys?
#define REALCONS_CPU_PDP11_CPUMODE_KERNEL	MD_KER
#define REALCONS_CPU_PDP11_CPUMODE_SUPERVISOR	MD_SUP
#define REALCONS_CPU_PDP11_CPUMODE_USER	MD_USR

#define REALCONS_CPU_PDP11_IDMODE_I	0
#define REALCONS_CPU_PDP11_IDMODE_D	1


typedef struct
{
	/* base class members, identical for all panels */
	struct realcons_struct *realcons; // parent
	// parent is used to link to other data structs:
	//  example: Blinkenlight API panel struct = realcons->blinkenlight_panel

	/* Signals for SimH's generic console interface
	 * (halt, start, singlestep, exam deposit).
	 * Signals in VHDL are variables holding values of physical wires
	 * Signals in REALCONS are pointers to variables either in the SimH simulation code, or
	 * in panel logic. They connect simulation and panel. A certain signal is can be unidirectional
	 * (either set by simulator and read by panel logic, ore vice verse) or be bidirectional.
	 */
		t_addr *cpusignal_memory_address_phys_register; // physical address of last bus cycle (EXAM/DEPOSIT)
        t_addr *cpusignal_memory_address_virt_register; // virtual address of last bus cycle (EXAM/DEPOSIT)
        char **cpusignal_register_name; // name of last accessed exam/deposit register
		t_value *cpusignal_memory_data_register; // data of last bus cycle
		int		*cpusignal_memory_write_access ; // is last memory accessa WRITE?
        t_stat *cpusignal_memory_status; // last memory access status
		int *cpusignal_console_halt; // 1, if a real console halts program execution
		volatile t_bool *cpusignal_run; // 1, if simulated cpu is running


	    /* Signals for SimH's PDP11 cpu. */

		t_value *cpusignal_DATAPATH_shifter; // value of shifter in PDP-11 processor data paths
	//	t_value *bus_register; // 11/70: BR - DATA of UNIBUS access
		int 	*cpusignal_bus_ID_mode ; // ID mode of last bus access. 1 = DATA space access, 0 = instruction space access
		unsigned *cpusignal_cpu_mode; // cpu run mode. which memory mode to display? REALCONS_CPU_PDP11_MEMMODE_*, MD_SUP,MD_KER,MD_USR,MD_UND
        int32   * cpusignal_MMR0 ; // MMU register 17 777 572
        int32   * cpusignal_MMR3 ; // MMU register 17 772 516

		int32 *cpusignal_PC; // programm counter
		t_value *cpusignal_PSW; // processor status word
		t_value *cpusignal_instruction_register; // cpu internal instruction register, holds opcode
		t_value *cpusignal_R0; // cpu register R0, needed for 11/40
		t_value *cpusignal_switch_register; // SR
		t_value *cpusignal_display_register; // DR

	/* 11/70 specific */
	// input controls on the panel
	    blinkenlight_control_t *keyswitch_power;
	    blinkenlight_control_t *keyswitch_panel_lock;

	blinkenlight_control_t *switch_SR, *switch_LOAD_ADRS, *switch_EXAM, *switch_DEPOSIT,
			*switch_CONT, *switch_HALT, *switch_S_BUS_CYCLE, *switch_START, *switch_DATA_SELECT,
			*switch_ADDR_SELECT, *switch_PANEL_LOCK;
	// output controls on the panel
	blinkenlight_control_t *leds_ADDRESS, *leds_DATA, *led_PARITY_HIGH, *led_PARITY_LOW,
			*led_PAR_ERR, *led_ADRS_ERR, *led_RUN, *led_PAUSE, *led_MASTER, *leds_MMR0_MODE,
			*led_DATA_SPACE, *led_ADDRESSING_16, *led_ADDRESSING_18, *led_ADDRESSING_22;

	// intern state registers of console panel
	unsigned run_state; // cpu can be: reset, halt, running.
	t_value loaded_address_15_00_datapathSR; // set by LOAD ADDR, bits 15:00, is data path SR and PCA
    // not persistent!
    t_value loaded_address_datapathSWR; // set by LOAD ADRS, bits 21:00, 21:16 + SR = CONS PHY


	// realcons_machine_word_t CPU_SHIFTER; // output of internal shifter.
	// result of calculations, everydata writtenten to Bus or other registers
	// realcons_machine_word_t CPU_BUSREGISTER ;// output of bus register
	// data EXAMed or DEPosited

	// auto address inc logic: next DEP or EXAM increases R_ADRSC
	blinkenlight_control_t *autoinc_addr_action_switch; // switchEXAM or switchDEP
	// NULL = no autoinc

    t_stat last_memory_status; // recognize changes in cpusignal_memory_status
} realcons_console_logic_pdp11_70_t;

realcons_console_logic_pdp11_70_t *realcons_console_pdp11_70_constructor(
		struct realcons_struct *realcons);
void realcons_console_pdp11_70_destructor(realcons_console_logic_pdp11_70_t *_this);

struct realcons_console_controller_interface_struct;
void realcons_console_pdp11_70_interface_connect(realcons_console_logic_pdp11_70_t *_this,
		struct realcons_console_controller_interface_struct *intf, char *panel_name);

int realcons_console_pdp11_70_test(realcons_console_logic_pdp11_70_t *_this, int arg);

// process panel state. operates only "panel" data struct
t_stat realcons_console_pdp11_70_service(realcons_console_logic_pdp11_70_t *_this);
t_stat realcons_console_pdp11_70_reset(realcons_console_logic_pdp11_70_t *_this);

#endif /* REALCONS_CONSOLE_PDP11_70_H_ */
