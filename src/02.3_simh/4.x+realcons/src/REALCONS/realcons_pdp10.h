/*  realcons_pdp10.h: State and function identical for the PDP10 KA10 and KI10 console

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


   31-Aug-2014  JH      created
*/


#ifndef REALCONS_PDP10_CPU_H_
#define REALCONS_PDP10_CPU_H_

#include "realcons_pdp10_control.h"

// Problem: cpu_mode constant are needed for "cm", but this include here
//      #include "pdp10_defs.h"
// damages the simulator,
// so define MD_* here identical to  pdp10_defs.h"
//#define MD_KER          0
//#define MD_SUP          1
//#define MD_UND          2
//#define MD_USR          3

// use mode codes for sim, because direct access to "cm"
//#define REALCONS_CPU_PDP11_CPUMODE_PHYS	MD_UND		// MD_UND is no where used, Phys?
//#define REALCONS_CPU_PDP11_CPUMODE_KERNEL	MD_KER
//#define REALCONS_CPU_PDP11_CPUMODE_SUPERVISOR	MD_SUP
//#define REALCONS_CPU_PDP11_CPUMODE_USER	MD_USR

//#define REALCONS_CPU_PDP11_IDMODE_I	0
//#define REALCONS_CPU_PDP11_IDMODE_D	1


/*** compact macros to be used in cpu_pdp10.c to register memory accesses ***/
// pa = physical address
// evaluate virtual address
// va: extended virtual address.
// bit 18,17 = cm = mode: MD_SUP, MD_KER,MD_USR,MD_UND, see calc_is()
// bit 16 = I/D space flag, see calc_ds(). 1 = data, 0 = instruction
// R/W access, only physical address given
#define REALCONS_CPU_PDP10_MEMACCESS_PA(realcons,pa,data_expr)	do { \
		REALCONS_SIGNAL_SET(realcons,signals_cpu_generic.bus_address_register, (pa)) ;	\
		REALCONS_SIGNAL_SET(realcons,signals_cpu_generic.bus_data_register, (data_expr)) ;	\
	} while(0)

// R/W access, virutal and physical address given
#define REALCONS_CPU_PDP10_MEMACCESS_VA_PA(realcons,va,pa,data_expr)	 do { \
			REALCONS_SIGNAL_SET(realcons,signals_cpu_pdp11.bus_ID_mode,((va) & 0x10000)? 1 : 0) ; \
			REALCONS_SIGNAL_SET(realcons,signals_cpu_generic.bus_address_register, (pa)) ;	\
			REALCONS_SIGNAL_SET(realcons,signals_cpu_generic.bus_data_register, (data_expr)) ;	\
		} while(0)



// Logic of the panel ... which is different electronic than the CPU.
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
		t_addr *cpusignal_memory_address_phys_register; // address of last bus cycle (EXAM/DEPOSIT)
		char **cpusignal_register_name; // name of last accessed exam/deposit register
		t_value *cpusignal_memory_data_register; // data of last bus cycle
		int *cpusignal_console_halt; // 1, if a real console halts program execution
		volatile t_bool *cpusignal_run; // 1, if simulated cpu is running

		 /* Signals for SimH's PDP10 cpu. */
		 int32 *cpusignal_PC; // programm counter
		 uint64_t	*cpusignal_instruction ; // current instruction

		 int32		*cpusignal_pi_on ; // PI system enable
		 int32_t	*cpusignal_pi_enb ; // PI levels enabled. simh pdp10 cpu reg "PIENB"
			 int32_t	*cpusignal_pi_act ; // PI levels active. simh  pdp10 cpu reg "PIACT"
			 int32_t	*cpusignal_pi_prq ; // PI levels with program requests . simh  pdp10 cpu reg "PIPRQ"
			 int32_t	*cpusignal_pi_ioq ; // PI levels with IO requests. simh  pdp10 cpu reg "PIIOQ"

	// KI10 console registers in SimH
	uint64_t  *cpusignal_console_data_switches ;
	uint64_t  *cpusignal_console_memory_indicator ;
	uint64_t  *cpusignal_console_address_addrcond ;


	/* KI10 specific Blinkenlight API controls */
	realcons_pdp10_control_t lamp_OVERTEMP ;
	realcons_pdp10_control_t lamp_CKT_BRKR_TRIPPED ;
	realcons_pdp10_control_t lamp_DOORS_OPEN ;
	realcons_pdp10_control_t knob_MARGIN_SELECT ;
	realcons_pdp10_control_t knob_IND_SELECT ;
	realcons_pdp10_control_t knob_SPEED_CONTROL_COARSE ;
	realcons_pdp10_control_t knob_SPEED_CONTROL_FINE ;
	realcons_pdp10_control_t knob_MARGIN_VOLTAGE ;
	realcons_pdp10_control_t VOLTMETER ;
	realcons_pdp10_control_t enable_HOURMETER ;
	realcons_pdp10_control_t button_FM_MANUAL ;
	realcons_pdp10_control_t buttons_FM_BLOCK ;
	realcons_pdp10_control_t buttons_SENSE ;
	realcons_pdp10_control_t button_MI_PROG_DIS ;
	realcons_pdp10_control_t button_MEM_OVERLAP_DIS ;
	realcons_pdp10_control_t button_SINGLE_PULSE ;
	realcons_pdp10_control_t button_MARGIN_ENABLE ;
	realcons_pdp10_control_t buttons_READ_IN_DEVICE ;
	realcons_pdp10_control_t buttons_MANUAL_MARGIN_ADDRESS ;
	realcons_pdp10_control_t button_LAMP_TEST ;
	realcons_pdp10_control_t button_CONSOLE_LOCK ;
	realcons_pdp10_control_t button_CONSOLE_DATALOCK ;
	realcons_pdp10_control_t button_POWER;

	realcons_pdp10_control_t leds_PI_ACTIVE ;
	realcons_pdp10_control_t leds_IOB_PI_REQUEST ;
	realcons_pdp10_control_t leds_PI_IN_PROGRESS ;
	realcons_pdp10_control_t leds_PI_REQUEST ;
	realcons_pdp10_control_t led_PI_ON ;
	realcons_pdp10_control_t led_PI_OK_8 ;
	realcons_pdp10_control_t leds_MODE ;
	realcons_pdp10_control_t led_KEY_PG_FAIL ;
	realcons_pdp10_control_t led_KEY_MAINT ;
	realcons_pdp10_control_t leds_STOP ;
	realcons_pdp10_control_t led_RUN ;
	realcons_pdp10_control_t led_POWER ;
	realcons_pdp10_control_t leds_PROGRAM_COUNTER ;
	realcons_pdp10_control_t leds_INSTRUCTION ;
	realcons_pdp10_control_t led_PROGRAM_DATA ;
	realcons_pdp10_control_t led_MEMORY_DATA ;
	realcons_pdp10_control_t leds_DATA ;

	realcons_pdp10_control_t button_PAGING_EXEC ;
	realcons_pdp10_control_t button_PAGING_USER ;

	realcons_pdp10_control_t buttons_ADDRESS ;  // switches 14..35
	realcons_pdp10_control_t button_ADDRESS_CLEAR ;
	realcons_pdp10_control_t button_ADDRESS_LOAD ;

	realcons_pdp10_control_t buttons_DATA ;
	realcons_pdp10_control_t button_DATA_CLEAR ;
	realcons_pdp10_control_t button_DATA_LOAD ;

	realcons_pdp10_control_t button_SINGLE_INST ;
	realcons_pdp10_control_t button_SINGLE_PULSER ;
	realcons_pdp10_control_t button_STOP_PAR ;
	realcons_pdp10_control_t button_STOP_NXM ;
	realcons_pdp10_control_t button_REPEAT ;
	realcons_pdp10_control_t button_FETCH_INST ;
	realcons_pdp10_control_t button_FETCH_DATA ;
	realcons_pdp10_control_t button_WRITE ;
	realcons_pdp10_control_t button_ADDRESS_STOP ;
	realcons_pdp10_control_t button_ADDRESS_BREAK ;
	realcons_pdp10_control_t button_READ_IN ;
	realcons_pdp10_control_t button_START ;
	realcons_pdp10_control_t button_CONT ;
	realcons_pdp10_control_t button_STOP ;
	realcons_pdp10_control_t button_RESET ;
	realcons_pdp10_control_t button_XCT ;
	realcons_pdp10_control_t button_EXAMINE_THIS ;
	realcons_pdp10_control_t button_EXAMINE_NEXT ;
	realcons_pdp10_control_t button_DEPOSIT_THIS ;
	realcons_pdp10_control_t button_DEPOSIT_NEXT ;

	// these two flags are referenced by all buttons as "disable"
	// "1" =
	// From doc: " the console data lock disables the data and sense switches;
	//	the console lock disables all other buttons except those that are mechanical"
	unsigned	console_lock ;
	unsigned	console_datalock ;

	// bool. Source of value in memory indicator (lower pabel, row 4)
	// 0: display caused by console exam/deposit
	// 1, if CPU has written the value
	unsigned 	memory_indicator_program  ; // MI PROG

	// intern state registers of console panel
	unsigned run_state; // cpu can be: reset, halt, running.
} realcons_console_logic_pdp10_t;




struct realcons_struct;

realcons_console_logic_pdp10_t *realcons_console_pdp10_constructor(
		struct realcons_struct *realcons);
void realcons_console_pdp10_destructor(realcons_console_logic_pdp10_t *_this);

void realcons_console_pdp10_interface_connect(realcons_console_logic_pdp10_t *_this,
		realcons_console_controller_interface_t *intf, char *panel_name);

t_stat realcons_console_pdp10_reset(realcons_console_logic_pdp10_t *_this);

t_stat realcons_console_pdp10_service(realcons_console_logic_pdp10_t *_this);

int realcons_console_pdp10_test(realcons_console_logic_pdp10_t *_this, int arg);


void realcons_console_pdp10_event_connect(realcons_console_logic_pdp10_t *_this) ;
void realcons_console_pdp10_event_disconnect(realcons_console_logic_pdp10_t *_this) ;


void realcons_console_pdp10_event_simh_deposit(realcons_console_logic_pdp10_t *_this, struct REG *reg) ;


// realcons_cpu_pdp10_maintpanel.c
t_stat realcons_console_pdp10_maintpanel_service(realcons_console_logic_pdp10_t *_this) ;

t_stat realcons_pdp10_operpanel_service(realcons_console_logic_pdp10_t *_this) ;


/*************************************************************************
 * Definitions private to realcons_pdp10* modules
 * */
#ifdef REALCONS_PDP10_C_


// state of the cpu
#define RUN_STATE_HALT_MAN	0	// halt by panel
#define RUN_STATE_HALT_PROG	1	// halt by opcode
//#define RUN_STATE_RESET	1
//#define RUN_STATE_WAIT	2
#define RUN_STATE_RUN		3


// indexes of used general timers
#define TIMER_TEST	0
// #define TIMER_DATA_FLASH	1
#define TIME_TEST_MS		3000	// self test terminates after 3 seconds
//#define TIME_DATA_FLASH_MS	50		// if DATA LEDs flash, they are ON for 1/20 sec

 // SHORTER macros for signal access
 // assume "realcons_console_logic_pdp10_t *_this" in context
#define SIGNAL_SET(signal,value) REALCONS_SIGNAL_SET(_this,signal,value)
#define SIGNAL_GET(signal) REALCONS_SIGNAL_GET(_this,signal)


#endif






#endif /* REALCONS_PDP10_CPU_H_ */
