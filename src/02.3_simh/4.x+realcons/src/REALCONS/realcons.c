/* realcons.c: Main frame work, glue logic.

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

   24-Mar-2018  JH      scp.c: speed up readline_p() if realcons is disconnected
   18-Jun-2016  JH      added PDP-15, with param "bootimage" file path
   23-Apr-2016  JH      added PDP-11/20
   19-Feb-2016  JH      bugfix in call to event_connect/disconnect
   25-Mar-2012  JH      created


   Information flow:
   ===================
   - Panel switches produce SimH commands
     Examples : Switch sequence LOADADDR, EXAM produces an "examine <addr>" string.
                Switch HALT produces ^E break char (if machine is running)
                Switch RUN produces "cont"
   - SimH manipulates registers, sets memory, reads memory, runs simulated processor
   - Changes on SimH machien are propagated to Realcons machine state :
     A duplicate machine state, so the console panel logic has something to visualize
   - Panel logic displays machine state with Lamps.
   - Panel is provided with computing time: SimH calls realcons_service()
  	 in interactive input loop
  	 and instruction execution loop

   Modularity:
   ===========
   - realcons_* can be used indepently of SimH, only realcons_simh is specific
   - Functions can be separated after Model-View-Controller model:
  	 1 console panel "model" is the Blinkenlight API data struct of panel and controls
     2 console panel "controller" is the logic which reacts on switches and drice the lights.
       This is implemented in SimH and here, in the realcons_* sources.
     3 console panel "view" is the physical console on the BlinkenBone, or a GUI simulation
       driven by another Blinkenlight API server.
   - no global variables, all data objects are "constructed" and destructed dynamically
   - Programming model is slightly object based.
   - all logic for a specfic console panel is located in an instance of cosnsole contrller in realcons_<machine>.c
   - console instance is constructed late in the "connect" command
   - machine propertys and service function calls have a 2-stage calls hierarchy:
   - base class is "realcons", subclass is "realcons_<machine>

    Scalability
   ============
   One machine may have more than one consoles/panel-
   PDP10: has console, and <n> rack headers.


   SimH CPU, console controller logic, and panel on Blinkenlight server:
   ==============================================================
   - For a complete system, 3 objects must fit together
     1) the CPU SimH simulated (as in "set cpu <type>", type = "11/40", "11/70", ...
     2) the realcons console control logic
  	 3) the physical panel connectet to BlinkenLight API server.
        The panel name is defined in the configuration file.
   - All console panels have a common interface,
     It is initliazed in realcons_init()
   - on SimH "set cpu <type>", the realcons_machine_type is also set,
  	 and also the default name for "console_name"
   - "console_name" can be overwritten, togheter with hostname.
 */
#ifdef USE_REALCONS

#define REALCONS_C_

#include <stdlib.h>
#include <time.h>

#include "sim_defs.h"
#include "realcons.h"

#include "rpc_blinkenlight_api.h"
#include "blinkenlight_api_client.h"

// global data
// At the moment, only ONE realcons per SimH is possible
// Later, every device unit may have its own ... may be ...
realcons_t *cpu_realcons;

/*
 *	constructor/destructor
 */
realcons_t *realcons_constructor(void)
{
	realcons_t * _this;
	_this = (realcons_t *)malloc(sizeof(realcons_t));

	realcons_init(_this);
	return _this;
}

void realcons_destructor(realcons_t *_this)
{
	realcons_disconnect(_this); // error tolerant, free blinkenlight_api_client and panels

	free(_this);
}

/*
 *	print string into SimHs output system
 */
void realcons_printf(realcons_t *_this, FILE *stream, const char *fmt, ...)
{
	char buff[2048]; // fixed size: very bad idea .....
	va_list args;
	va_start(args, fmt);

	vsnprintf(buff, sizeof(buff), fmt, args);
	fputs(buff, stream);
}

/*
 *	global initialization
 *	connection to host/console panel is not yet done here!
 *  disconnect interface
 *  CPU/device extensions must be initialized in SimH-code!
 */
void realcons_init(realcons_t *_this)
{
	int i;

	// clear all data, including the interface
	memset(_this, 0, sizeof(*_this));

	strcpy(_this->application_server_hostname, "localhost");
	_this->console_logic_name[0] = '\0';
	_this->application_panel_name[0] = '\0';
    _this->boot_image_filepath[0] = '\0';
	_this->blinkenlight_api_client = NULL; /// created in connect()
	_this->connected = 0;

	_this->service_interval_msec = REALCONS_DEFAULT_SERVICE_INTERVAL_MSEC;
	_this->service_highspeed_prescaler = 0;
	_this->service_next_time_msec = 0;

	/* Intializes random number generator */
	srand((unsigned)time(NULL));

	for (i = 0; i < REALCONS_TIMER_COUNT; i++)
		_this->timer_running_msec[i] = 0;

	_this->debug = 0;

#ifdef VM_PDP11
	// is set in pdp11_cpumod.c, depending on CPU model
#endif
#ifdef VM_PDP10
	// there's a separate panel for every PDP11 model,
	// but only one is hardwired to the PDP10.
	// Check the service configuration file "/etc/blinkenlightd.conf" on panel host
	strcpy(_this->console_logic_name, "PDP10-KI10"); // overwritten by const
	strcpy(_this->application_panel_name, "PDP10-KI10");
#endif
#ifdef VM_PDP8
	// name of the panel: defaults to "PiDP8" for Oscars device.
	strcpy(_this->console_logic_name, "PDP8"); // overwritten by const
	strcpy(_this->application_panel_name, "PiDP8");
#endif
#ifdef PDP15
	// name of the panel: defaults to "PDP15" for blinkenlight server
	strcpy(_this->console_logic_name, "PDP15");
	strcpy(_this->application_panel_name, "PDP15");
#endif
}

/*
 * connect to hostname, consolename
 * "console_controllername" is used to search the panel logic
 * "console_viewname" is used to search the panel on the Blinkenlight API server
 */
t_stat realcons_connect(realcons_t *_this, char *consolelogic_name, char *server_hostname,
	char *panel_name)
{
	t_stat reason;
	unsigned  i;
	blinkenlight_panel_list_t *pl;

	if (_this->connected)
		return SCPE_ALATT; // already attached

	_this->connected = 0;
	_this->console_model = NULL;
	_this->console_controller = NULL;
	// invalidate interface.
	memset(&(_this->console_controller_interface), 0, sizeof(_this->console_controller_interface));

	_this->force_output_update = 0;
	_this->service_highspeed_prescaler = 0;
	_this->service_next_time_msec = 0;
	_this->service_cycle_count = 0;
	_this->lamp_test = 0;
	for (i = 0; i < REALCONS_TIMER_COUNT; i++)
		_this->timer_running_msec[i] = 0;

	if (server_hostname == NULL || server_hostname[0] == 0) {
		realcons_printf(_this, stdout, "Realcons hostname not set!\n");
		return SCPE_NOATT; // illegal parameter
	}
	if (consolelogic_name == NULL || consolelogic_name[0] == 0) {
		realcons_printf(_this, stdout, "Realcons console controller not set!\n");
		return SCPE_NOATT; // illegal parameter
	}

	if (panel_name == NULL || panel_name[0] == 0) {
		realcons_printf(_this, stdout, "Realcons console name not set!\n");
		return SCPE_NOATT; // illegal parameter
	}

	/* search console_controller */
	realcons_printf(_this, stdout, "Searching realcons controller \"%s\" ...\n",
		consolelogic_name);
	{
		// connect signals which are independent of console logic
		realcons_console_controller_interface_t *intf = &(_this->console_controller_interface);
		_this->console_controller = NULL;
#ifdef VM_PDP11
        if (!strcmp(consolelogic_name, "11/20")) {
			realcons_console_logic_pdp11_20_t *console_logic;
			// 1. create
			console_logic = realcons_console_pdp11_20_constructor(_this);
			// 2. connect
			realcons_console_pdp11_20_interface_connect(console_logic,
				&(_this->console_controller_interface), consolelogic_name);
			_this->console_controller = console_logic;
		}
		else if (!strcmp(consolelogic_name, "11/40")) {
			realcons_console_logic_pdp11_40_t *console_logic;
			// 1. create
			console_logic = realcons_console_pdp11_40_constructor(_this);
			// 2. connect
			realcons_console_pdp11_40_interface_connect(console_logic,
				&(_this->console_controller_interface), consolelogic_name);
			_this->console_controller = console_logic;
		}
		else if (!strcmp(consolelogic_name, "11/70")) {
			realcons_console_logic_pdp11_70_t *console_logic;
			// 1. create
			console_logic = realcons_console_pdp11_70_constructor(_this);
			// 2. connect
			realcons_console_pdp11_70_interface_connect(console_logic,
				&(_this->console_controller_interface), consolelogic_name);
			_this->console_controller = console_logic;
		}
#endif
#ifdef VM_PDP10
		// no selection, always "PDP10-KI10"
		{
			realcons_console_logic_pdp10_t *console_logic;
			// 1. create
			console_logic = realcons_console_pdp10_constructor(_this);
			// 2. connect
			realcons_console_pdp10_interface_connect(console_logic,
				&(_this->console_controller_interface), consolelogic_name);
			_this->console_controller = console_logic;
		}
#endif
#ifdef VM_PDP8
		// no selection, always "PDP8I"
		{
			realcons_console_logic_pdp8i_t *console_logic;
			// 1. create
			console_logic = realcons_console_pdp8i_constructor(_this);
			// 2. connect
			realcons_console_pdp8i_interface_connect(console_logic,
				&(_this->console_controller_interface), consolelogic_name);
			_this->console_controller = console_logic;
		}
#endif
#ifdef PDP15
		// no selection, always "PDP15"
		{
			realcons_console_logic_pdp15_t *console_logic;
			// 1. create
			console_logic = realcons_console_pdp15_constructor(_this);
			// 2. connect
			realcons_console_pdp15_interface_connect(console_logic,
				&(_this->console_controller_interface), consolelogic_name);
			_this->console_controller = console_logic;
		}
#endif



		if (!_this->console_controller) {
			realcons_printf(_this, stdout, "No controller for console \"%s\"!\n",
				consolelogic_name);
			return SCPE_NOATT;
		}
	}
	if (!_this->console_controller) {
		realcons_printf(_this, stdout, "Instanciating  controller for console \"%s\" failed!\n",
			consolelogic_name);
		return SCPE_NOATT;
	}

	realcons_printf(_this, stdout, "Connecting to host %s ...\n", server_hostname);

	_this->blinkenlight_api_client = blinkenlight_api_client_constructor();

	if (blinkenlight_api_client_connect(_this->blinkenlight_api_client, server_hostname) != 0) {
		realcons_printf(_this, stdout, "Connect to host %s failed.\n", server_hostname);
		return SCPE_NOATT;
	}

	// load defined console panels and controls from server into global "blinkenlight_panel_list"
	if (blinkenlight_api_client_get_panels_and_controls(_this->blinkenlight_api_client) != 0) {
		realcons_printf(_this, stdout, "Loading panels and controls from host %s failed.\n",
			server_hostname);
		return SCPE_NOATT;
	}
	pl = _this->blinkenlight_api_client->panel_list; //alias
	// now try to find the panel with the right name
	_this->console_model = blinkenlight_panels_get_panel_by_name(pl, panel_name);
	if (_this->console_model == NULL) {
		realcons_printf(_this, stdout, "Panel \"%s\" not published by server \"%s\".\n",
			panel_name, server_hostname);
		// show all panels the server publishes
		realcons_printf(_this, stdout, "  Published panels are: ");
		for (i = 0; i < pl->panels_count; i++) {
			if (i > 0)
				realcons_printf(_this, stdout, ", ");
			realcons_printf(_this, stdout, "\"%s\"", pl->panels[i].name);
		}
		realcons_printf(_this, stdout, "\n");

		realcons_disconnect(_this);
		return SCPE_OPENERR;
	}

	// query inputs the first time. crash exit on error
	if (blinkenlight_api_client_get_inputcontrols_values(_this->blinkenlight_api_client,
		_this->console_model) != 0) {
		realcons_printf(_this, stdout, "Can not read inputs of console \"%s\" on server \"%s\"\n",
			panel_name, server_hostname);
		return SCPE_OPENERR;
	}

	// setup console panel controller state
	reason = _this->console_controller_interface.reset_func(_this->console_controller);
	if (reason != SCPE_OK)
		return reason;

	_this->connected = 1;
	_this->force_output_update = 1; // transmit output control to console panel regardless of changes

	// tell server to enable the output driver of the BlinkenBoards.
	// The BlinkenBoards are tristate, if they had a power loss since
	// start of the server.
	blinkenlight_api_client_set_object_param(_this->blinkenlight_api_client, RPC_PARAM_CLASS_PANEL,
		_this->console_model->index, RPC_PARAM_HANDLE_PANEL_BLINKENBOARDS_STATE,
		RPC_PARAM_VALUE_PANEL_BLINKENBOARDS_STATE_ACTIVE);

	// notify panel on connect
	if (_this->console_controller_interface.event_connect)
		_this->console_controller_interface.event_connect(_this->console_controller);

	// initialize console panel state to HALT mode.
	// TODO: move to scp.c ?
	{
		extern 	console_controller_event_func_t	realcons_event_operator_halt; // after cpu is stopped by operator
		REALCONS_EVENT(cpu_realcons, realcons_event_operator_halt);
	}

	// update outputs immediately
	blinkenlight_api_client_set_outputcontrols_values(_this->blinkenlight_api_client,
		_this->console_model);

	return SCPE_OK;
}


t_stat realcons_disconnect(realcons_t *_this)
{
	if (!_this->connected)
		return SCPE_OK; // not attached, error tolerant

	// notify panel on disconnect
	if (_this->console_controller_interface.event_disconnect)
		_this->console_controller_interface.event_disconnect(_this->console_controller);

	// update outputs immediately
	blinkenlight_api_client_set_outputcontrols_values(_this->blinkenlight_api_client,
		_this->console_model);

	//// console panel struct remains valid, but links are cleared
	// NO rpc_clientfree()/destroy() needed???
	_this->console_model = NULL;
	_this->connected = 0;
	// now EVENT() does not call any callbacks. The

	// cleanup rpc subsystem
	blinkenlight_api_client_disconnect(_this->blinkenlight_api_client);
	blinkenlight_api_client_destructor(_this->blinkenlight_api_client);
	// free console controller
	_this->console_controller_interface.destructor_func(_this->console_controller);
	_this->console_controller = NULL;
	// invalidate interface.
	memset(&(_this->console_controller_interface), 0, sizeof(_this->console_controller_interface));

	_this->blinkenlight_api_client = NULL;

	return SCPE_OK;
}

/*
 * Scheduling:
 * Provide realcons with computing time.
 * update periodically lamps and switches
 *
 * highspeed: 1: is called in tight loop,
 *	even check for timeout is to
 */
void realcons_service(realcons_t *_this, int highspeed)
{
	int i;
    if (!_this->connected)
        return;

	// if called by high speed loop: check only
	if (highspeed && _this->service_highspeed_prescaler > 0) {
		_this->service_highspeed_prescaler--;
		return;
	}
	_this->service_highspeed_prescaler = REALCONS_SERVICE_HIGHSPEED_PRESCALE; // reload

	// sample current time. can be used in console panel subclasses->service()
	_this->service_cur_time_msec = sim_os_msec(); // get current time in millisec
	// update general purpose timers.
	for (i = 0; i < REALCONS_TIMER_COUNT; i++)
		if (_this->timer_running_msec[i]
			&& _this->timer_running_msec[i] < _this->service_cur_time_msec)
			_this->timer_running_msec[i] = 0; // timer expired

	if (_this->service_next_time_msec >= _this->service_cur_time_msec)
		return;
	///// Time for next service operation /////

	if (_this->connected)
		_this->service_cycle_count++;
	//// do your jobs only if not disconnected
	if (_this->connected) {
		// 1) query new input values from console
		if (blinkenlight_api_client_get_inputcontrols_values(_this->blinkenlight_api_client,
			_this->console_model) != 0) {
			// error in service: disconnect
			realcons_printf(_this, stderr,
				blinkenlight_api_client_get_error_text(_this->blinkenlight_api_client));
			realcons_disconnect(_this);
		}
	}

	if (_this->connected) {
		// 2) process console state. operates only "panel" model data struct
		_this->console_controller_interface.service_func(_this->console_controller);
	}

	if (_this->connected) {
		unsigned n = blinkenlight_panels_get_control_value_changes(
			_this->blinkenlight_api_client->panel_list, _this->console_model, /*output*/0);
		if (n > 0 || _this->force_output_update) {
			if (_this->debug)
				printf("realcons_server(): outputcontrols\n");
			// 3) transmit output control values to panel
			// Transmit not if nothing changed.
			//always: server panel simulation may relay on display clock!
			_this->force_output_update = 0; // done
			if (blinkenlight_api_client_set_outputcontrols_values(_this->blinkenlight_api_client,
				_this->console_model) != 0) {
				// error in service: disconnect
				realcons_printf(_this, stderr,
					blinkenlight_api_client_get_error_text(_this->blinkenlight_api_client));
				realcons_disconnect(_this);
			}
		}
	}

	// 2 options
	// a) try to run service exactly at REALCONS_SERVICE_INTERVAL_MSEC
	// b) run service with pauses of minimal REALCONS_SERVICE_INTERVAL_MSEC
	// realcons->service_next_time_msec += REALCONS_SERVICE_INTERVAL_MSEC ; // do not run exact

	// set next execution time, AFTER all work is done
	_this->service_next_time_msec = sim_os_msec()
		+ (_this->debug ? REALCONS_SERVICE_INTERVAL_DEBUG_MSEC : _this->service_interval_msec)
		/*No  random term to avoid visual interferences with panel LEDs and CPU loops.
		  Better solution is LED low pass in Blinkenlight API servers
         */
  	    //		+ (rand() % 2) /* + 0..1 msecs*/
	    //			+ (rand() % 10) - 5 /* +/- 5 msecs*/

;
}

/*
 * stop simulated cpu, but keep console logic running
 * (needed for PDP-11 RESET opcode)
 */
void realcons_ms_sleep(realcons_t *_this, int ms)
{
	uint32 end_time_msec;

	end_time_msec = sim_os_msec() + ms; // get current time in millisec
	while (end_time_msec > sim_os_msec()) {
		// busy waiting
		realcons_service(_this, 0);
	}
}

/*
 * get a control over name
 */
blinkenlight_control_t *realcons_console_get_input_control(realcons_t *_this, char *controlname)
{
	blinkenlight_control_t *c;
	if (_this->console_model == NULL) {
		realcons_printf(_this, stderr, "Required console not loaded\n");
		return NULL;
	}
	c = blinkenlight_panels_get_control_by_name(_this->blinkenlight_api_client->panel_list,
		_this->console_model, controlname, /*is_input*/1);
	if (c == NULL) {
		realcons_printf(_this, stderr, "Required input control \"%s\" not found\n", controlname);
		return NULL;
	}
	return c;
}

blinkenlight_control_t * realcons_console_get_output_control(realcons_t *_this, char *controlname)
{
	blinkenlight_control_t *c;
	if (_this->console_model == NULL) {
		realcons_printf(_this, stderr, "Required console not loaded\n");
		return NULL;
	}
	c = blinkenlight_panels_get_control_by_name(_this->blinkenlight_api_client->panel_list,
		_this->console_model, controlname, /*is_input*/0);
	if (c == NULL) {
		realcons_printf(_this, stderr, "Required output control \"%s\" not found\n", controlname);
		return NULL;
	}
	return c;
}

#ifdef USED
// clear all controls to state 0, wether they are used by panel logic or not.
// State == 0 means NOT "all LEDs power-less"!
// Example: a control may consist two LEDs labled with "feature ON" and feature OFF"
//	than state == would light "OFF".
void	realcons_console_clear_output_controls(realcons_t *_this)
{
	blinkenlight_panel_t *p = _this->console_model ;
	unsigned i_control ;
		for (i_control = 0; i_control < p->controls_count; i_control++)	{
				blinkenlight_control_t *c = &(p->controls[i_control]);
			if (!c->is_input)
				c->value = 0 ; // oder c->value = c->defaultvalue ??
			}
}
#endif


/*
 *	produce a simh command line from console input
 *  Example:
 *   	console was operated LOADADDR, EXAM
 * 	    result string = "examine <address>"
 *   - this is merged with stdin/telnet input py simh's "read_line()"
 *   - if the console produced output:
 * 	this can be used by SimH to supress additional "simh>" prompts
 *	cptr = read_line (gbuf, CBUFSIZE, stdin);
 *
 * simh_cmd_buffer may contain multiple lines, separates by \n
 * this function returns the content line by line.
 * cmd is cleared after call!
 */

// for console emulator: add cmd string. caller must set "\n"
void realcons_simh_add_cmd(realcons_t *_this, char *format, ...) {
    char    cmd[256];
    va_list argptr;
    va_start(argptr, format);
    vsprintf(cmd, format, argptr);
    strcat(_this->simh_cmd_buffer, cmd);
}


char *realcons_simh_get_cmd(realcons_t *_this)
{
	static char buffer[SIMH_CMDBUFFER_SIZE]; // static buffer for result
	char *eolpos;
	if (!_this->connected)
		return NULL;
	if (strlen(_this->simh_cmd_buffer) == 0)
		return NULL; // no news
	// find position of first \n
	eolpos = strchr(_this->simh_cmd_buffer, '\n');
	if (!eolpos) {
		strcpy(buffer, _this->simh_cmd_buffer); // return cmd ...
		_this->simh_cmd_buffer[0] = '\0'; // ... and mark it as "processed"
	}
	else {
		char *wp, *rp;
		// return first line, WITH appended '\n'
		wp = buffer ;
		rp = _this->simh_cmd_buffer;
		while (*rp && *rp != '\n')
			*wp++ = *rp++ ; // copy first line to result buffer
		if (*rp)
			*wp++ = *rp++ ; // copy the '\n'
		*wp = 0 ; // terminate result
		// rp now on first char after \n
		wp = _this->simh_cmd_buffer;
		// shift folowing lines to start of bufer (if any)
		while ((*wp++ = *rp++))
			;
	}
	return buffer;
}


/*
 * return the next char from the command buffer, and delete it.
 */
char realcons_simh_getc_cmd(realcons_t *_this)
{
	char result ;
	char *wp, *rp;
	if (!_this->connected)
		return EOF;
	if (strlen(_this->simh_cmd_buffer) == 0)
		return EOF; // no data
	result = _this->simh_cmd_buffer[0] ;
	// shift buffer content
	wp = _this->simh_cmd_buffer;
	rp = wp + 1;
	while ((*wp++ = *rp++)) ;
	return result ;
}

// call self test. 0 = OK, else (undefd) error
int realcons_test(realcons_t *_this, int arg)
{
	int	result;
	if (_this->connected)
		result = _this->console_controller_interface.test_func(_this->console_controller, arg);
	else
		result = 1; // not connected
	realcons_printf(_this, stdout, "REALCONS device for blinkenlight panels. Build " __DATE__ ".\n");
	realcons_printf(_this, stdout, "Contact: www.retrocmp.com; j_hoppe@t-online.de\n");

	return result;
}

// set the panel into testmode
// mode = 1: everything on, 0 = normal
void realcons_lamp_test(realcons_t *_this, int testmode)
{
	if (testmode) {
		_this->lamp_test = 1;
		_this->console_model->mode = 1; // "realistic" test
	}
	else {
		_this->lamp_test = 0;
		_this->console_model->mode = 0; // back to normal
	}
	blinkenlight_api_client_set_object_param(_this->blinkenlight_api_client,
		RPC_PARAM_CLASS_PANEL, _this->console_model->index, RPC_PARAM_HANDLE_PANEL_MODE,
		_this->console_model->mode);
}


// set the panel to normal (1) or power off (0)
void realcons_power_mode(realcons_t *_this, int power_on) {
	if (power_on)
	  _this->console_model->mode = RPC_PARAM_VALUE_PANEL_MODE_NORMAL;
	else
	_this->console_model->mode = RPC_PARAM_VALUE_PANEL_MODE_POWERLESS;
blinkenlight_api_client_set_object_param(_this->blinkenlight_api_client,
	RPC_PARAM_CLASS_PANEL, _this->console_model->index, RPC_PARAM_HANDLE_PANEL_MODE,
	_this->console_model->mode);
}


/* called on any "deposit", to convert register changes into events  */
void realcons_simh_event_deposit(realcons_t *_this, struct REG *reg)
{
	if (!_this->connected)
		return;
#ifdef VM_PDP10
	// route to PDP10 panel
	realcons_console_pdp10_event_simh_deposit(
		(realcons_console_logic_pdp10_t *)(_this->console_controller), reg);
#endif
}

#endif // USE_REALCONS

