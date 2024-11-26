/* realcons.h: Main frame work, glue logic.

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

   18-Jun-2016  JH      added PDP-15, with param "bootimage" file path
   23-Apr-2016  JH      added PDP-11/20
   20-Feb-2016  JH      added PANEL_MODE_POWERLESS
   25-Mar-2012  JH      created
*/


#ifndef REALCONS_H_
#define REALCONS_H_

#include "sim_defs.h"		// types
 // #define so dass rpc kram gekapselt wird
 // void *CLIENT
#include "blinkenlight_api_client.h" // RPC and panel list provided by server
// include all realcons_* modules
#include "realcons_simh.h"

#ifdef WIN32
  #ifndef strcasecmp
    #define strcasecmp _stricmp	// grmbl
  #endif
#else
#include <limits.h> // INT_MAX
#endif



// base type for machine registers
typedef uint32 realcons_machine_word_t;


#define REALCONS_NAMELEN	1024
#define REALCONS_INFOLEN	1024
#define SIMH_CMDBUFFER_SIZE	1024

#define REALCONS_SERVICE_HIGHSPEED_PRESCALE 100	// optimization: reduced service interval
#define REALCONS_SERVICE_INTERVAL_DEBUG_MSEC  500 // time for diag output: 2 times per sec
#define REALCONS_DEFAULT_SERVICE_INTERVAL_MSEC  20 // run service 50 time per sec
#define REALCONS_TIMER_COUNT 4 // general purpose timers for use by console_controller
// states of the simulated machine

/*
 * description of logic procedures of an implemented console panel
 */
 // panel logic behind lights and switches
struct realcons_struct;
// type forward
typedef void * (*console_controller_constructor_func_t)(struct realcons_struct *realcons); // out: pointer to panel, in: parent realcons
typedef void(*console_controller_destructor_func_t)(void *console_controller);
// reset console panel and clear machine state
typedef t_stat(*console_controller_reset_func_t)(void *console_controller);
// provide console panel with computing time
typedef t_stat(*console_controller_service_func_t)(void *console_controller);
/*
 * control of machine state behind console panel logic
 */

 // a generic event callback. implemented by panel logic, called by simulator
typedef void(*console_controller_event_func_t)(void *console_controller);

// test cycle: lamps 1 sec on, print state of switches to stderr
typedef int(*console_controller_test_func_t)(void *console_controller, int arg);



// get a signal value, if connected
// requieres realcons_console_logic_xxx_t *_this in context
// Example: REALCONS_SIGNAL_GET(cpu_realcons, signals_cpu_pdp11.DATAPATH_shifter)
#define REALCONS_SIGNAL_GET(console_logic,signal) \
	( (console_logic)->signal? 	*(console_logic)->signal:0)
#define REALCONS_SIGNAL_SET(console_logic,signal,value)	do {	\
		if ((console_logic)->signal) *((console_logic)->signal) = (value) ;	\
	} while(0)



// call event callback with console_controller as argument
// no call, if console is disconnect. so invalid event pointer are no problem
// events are called in SimH-code as pointers to functions  in panel logic
// Example: REALCONS_EVENT(cpu_realcons, events_cpu_generic.opcode_any)
#define REALCONS_EVENT(realcons,event) do {\
		if ((realcons)->connected && (event) != NULL) (event)((realcons)->console_controller) ;	\
	} while(0)


typedef struct realcons_console_controller_interface_struct
{
	char *name; /* name, like in config file */

	console_controller_constructor_func_t constructor_func;
	console_controller_destructor_func_t destructor_func;
	console_controller_reset_func_t reset_func;
	console_controller_service_func_t service_func; // process state
	console_controller_test_func_t test_func;

	// event handler for pannels, to be called on connect/disconnect
	console_controller_event_func_t	event_connect; // panel was connected to device
	console_controller_event_func_t	event_disconnect; // panel was disconnected device

} realcons_console_controller_interface_t;

// all implemented console panels
#ifdef VM_PDP11
#include "realcons_console_pdp11_20.h"
#include "realcons_console_pdp11_40.h"
#include "realcons_console_pdp11_70.h"
#endif
#ifdef VM_PDP10
#include "realcons_pdp10.h"
#endif
#ifdef VM_PDP8
#include "realcons_console_pdp8i.h"
#endif
#ifdef PDP15
#include "realcons_console_pdp15.h"
#endif

// global data
typedef struct realcons_struct
{
	// scratch vars for application (= SimH), set before connect
	char console_logic_name[REALCONS_NAMELEN + 1]; // ID for panel logic class to instanciate.
	char application_server_hostname[REALCONS_NAMELEN + 1]; // TCP/IP name of Blinkenlight API server
	char application_panel_name[REALCONS_NAMELEN + 1]; // Name of panel on Blinkenlight server, set by configuration file

    // used for PDP-15 "READ-IN"  command
    char boot_image_filepath[REALCONS_NAMELEN + 1]; 


	blinkenlight_api_client_t * blinkenlight_api_client;
	int connected; // 1: Connected to Blinkenlight_API and logic
	int debug; // 1: debug mode

	// cmd for Simh, to be queried by realcons_simh_get_cmd()
	char simh_cmd_buffer[SIMH_CMDBUFFER_SIZE + 1];



#ifdef VM_PDP11
	// 2 different PDP11 CPUs
	// state variables for every support console panel type
	// life cycle between "realcons connect" and "realcons disconnect"
	union
	{
		realcons_console_logic_pdp11_40_t pdp11_40;
		realcons_console_logic_pdp11_70_t pdp11_70;
	} console_state_pdp11;
#endif

#ifdef VM_PDP10
#endif

#ifdef VM_PDP8
// 2. different PDP8 CPUs
// state variables for every support console panel type
// life cycle between "realcons connect" and "realcons disconnect"
#ifdef USEDxxxx
	union
	{
		realcons_console_logic_pdp8i_t pdp8i;
	} console_state_pdp8;
#endif

#ifdef VM_PDP15
#endif


#endif


	// int machine_state; // result of last call to machine_set_state()

	int force_output_update; // 0: transmit outputs to real console panel only if values have changed

	int lamp_test; // 1, if console panel should switch all lamps ON

	// panel logic (functional behaviour of lights & switches)
	realcons_console_controller_interface_t console_controller_interface; // description of logic module

	// panel over blinkenlight API (lights & switches), data state
	blinkenlight_panel_t *console_model;

	// data struct of console_controller, includes machine state
	void *console_controller;

	// in the "Model-View-Controller" paradigm, where is "console_view"?
	// It is the physical console panel or it's simulation, on the Blinkenlight APi server side!

	// service
	// period between two service cycles: update frequency for GUI and servers
	unsigned service_interval_msec; // limit for service() frequency.
	t_uint64 service_cycle_count; // inc by one for every seervice() call.
	int service_highspeed_prescaler; // check not every time for next service time
	t_uint64 service_next_time_msec; // next execution time for service in ms

	t_uint64 service_cur_time_msec; // current timestamp of service call

	// array of general purpose timers for use by console_controller. 0 = expired
	t_uint64 timer_running_msec[REALCONS_TIMER_COUNT];

} realcons_t;

#ifndef REALCONS_C_
// the ONE global real cconsole ... to be changed ...
extern realcons_t *cpu_realcons;
#endif

realcons_t *realcons_constructor(void);
void realcons_destructor(realcons_t *_this);

void realcons_init(realcons_t *_this);

void realcons_service(realcons_t *_this, int highspeed);

void realcons_ms_sleep(realcons_t *_this, int ms);

void realcons_printf(realcons_t *_this, FILE *stream, const char *fmt, ...);

t_stat realcons_connect(realcons_t *_this, char *console_controllername, char *server_hostname,
	char *console_viewname);
t_stat realcons_disconnect(realcons_t *_this);

blinkenlight_control_t *realcons_console_get_input_control(realcons_t *_this, char *name);
blinkenlight_control_t *realcons_console_get_output_control(realcons_t *_this, char *name);
void	realcons_console_clear_output_controls(realcons_t *_this) ;

void realcons_simh_add_cmd(realcons_t *_this, char *format, ...);
char *realcons_simh_get_cmd(realcons_t *_this); // has console panel controller generated a simh cmd string?
char realcons_simh_getc_cmd(realcons_t *_this) ; // next char of cmd strings

// test cycle: lamps 1 sec on, print state of switches to stderr
int realcons_test(realcons_t *_this, int arg);

void realcons_lamp_test(realcons_t *_this, int testmode);
void realcons_power_mode(realcons_t *_this, int power_on);

void realcons_simh_event_deposit(realcons_t *_this, struct REG *reg);


#endif /* REALCONS_H_ */
