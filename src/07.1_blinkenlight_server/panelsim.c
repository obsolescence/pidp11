/* panelsim.c: simulated panel implemented with interactive GUI

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

 23-Feb-2016  JH      added PANEL_MODE_POWERLESS
 	 	 	 	 	  added logic for const input controls
 17-Mar-2012  JH      created


 Drives the interactive GUI with the simulated panel
 Entry points:
 - the RPC server calls panelsim_read_panel_input_controls()
 and panelsim_write_panel_output_controls()
 - on idle, periodically panelsim_service() is called
 - user interface over command line

 Internals:
 - The state of inputs and output controls is already in
 blinkenlight_panels.blinkenlight_panel_list[]
 - only one of the panels in  blinkenlight_panel_list[] is
 simulated, that is hold in the pointer 'panelsim_panel'

 */
#define PANELSIM_C_

// DEBUG_BLINKENBUSFILE_LOCAL
// debugging without running driver.
// if defined: to not use /dev/blinkenbus, but
//	open,close() - just printlog()
//	write()- just printlog() addresses and data
//	read()- generate data, printlog() addresses and data
//#define DEBUG_BLINKENBUSFILE_LOCAL

#include <stdlib.h>
#include <stdio.h>
#include <string.h>	// memcpy()
#include <assert.h>
#ifdef WIN32
#include <windows.h>
#include <time.h>
#define strcasecmp _stricmp	// grmbl
#else
#include <sys/time.h>
#endif

#include "errno2txt.h"
#include "kbhit.h"
#include "print.h"
#include "bitcalc.h"
#include "radix.h"
#include "mcout.h"

#include "rpc_blinkenlight_api.h" // param value const
#include "blinkenlight_panels.h"

#include "main.h" // global blinkenlight_panel_list
#include "panelsim.h" // own definitions
int panelsim_menu_linewidth = 132; // screen width for menu display

#define MIN_UPDATE_INTERVALL_MS 500 // update display every 500ms = 2 times per second
// THE panel, which is simualted
static blinkenlight_panel_t *panelsim_panel;

static int panelsim_panel_drivers_state; // dummy flag: simlates BlinkenBoard driver tristate

// server wants display to update
static int update_needed = 0;
//static int user_input_in_progress = 0; // user is typing something
static char user_input_buffer[80];
static int user_input_chars = 0;

static struct timeval update_last_time_tv;
static unsigned screen_number = 0; // global count up display updates
/*
 * read character string from stdin
 */
static char *getchoice()
{
	static char s_choice[255];
	char *s;
	s_choice[0] = '\0'; //  empty buffer.
	fgets(s_choice, sizeof(s_choice), stdin);
	// clr \n
	for (s = s_choice; *s; s++)
		if (*s == '\n')
			*s = '\0';
	return s_choice;
}

/*
 * print out/update the content of input and output controls
 */
static void panelsim_show(void)
{
	unsigned i_control;
	unsigned i_incontrol;
	unsigned name_len;
	blinkenlight_control_t *c;
	mcout_t mcout; // Multi Column OUTput

	// no "change" marker ... why? copy&paste error?
	for (i_control = 0; i_control < panelsim_panel->controls_count; i_control++) {
		c = &(panelsim_panel->controls[i_control]);
		c->value_previous = c->value;
	}
	printf("\n");
	printf("\n");
	printf("*** Simulation of panel \"%s\" ***\n", panelsim_panel->name);
	printf("* Loaded config file: %s\n", configfilename);
	printf("* Update interval is %d ms, screen #%d\n", MIN_UPDATE_INTERVALL_MS, screen_number++);
	printf("* Simulated BlinkenBoard drivers: %s",
			panelsim_panel_drivers_state == RPC_PARAM_VALUE_PANEL_BLINKENBOARDS_STATE_ACTIVE ?
					"ACTIVE" :
					(panelsim_panel_drivers_state
							== RPC_PARAM_VALUE_PANEL_BLINKENBOARDS_STATE_TRISTATE ?
							"TRISTATE" : "OFF"));
	printf(", panel mode = 0x%x\n", panelsim_panel->mode);

	// first output controls, with value, but without item selectors

	for (name_len = i_control = 0; i_control < panelsim_panel->controls_count; i_control++) {
		c = &(panelsim_panel->controls[i_control]);
		if (!c->is_input && name_len < strlen(c->name))
			name_len = strlen(c->name);
	}
	printf("* Outputs:\n");
	mcout_init(&mcout, MAX_BLINKENLIGHT_PANEL_CONTROLS);
	for (i_control = 0; i_control < panelsim_panel->controls_count; i_control++) {
		c = &(panelsim_panel->controls[i_control]);
		if (!c->is_input) {
			uint64_t displayvalue;

			if (panelsim_panel->mode == RPC_PARAM_VALUE_PANEL_MODE_ALLTEST
					|| (panelsim_panel->mode == RPC_PARAM_VALUE_PANEL_MODE_LAMPTEST
							&& c->type == output_lamp)) {
				// test only LAMPS, should same as historic lamp test
				displayvalue = BitmaskFromLen64[c->value_bitlen];
				// "selftest": show "all ones"
			} else if (panelsim_panel->mode == RPC_PARAM_VALUE_PANEL_MODE_POWERLESS
					&& c->type == output_lamp)
				displayvalue = 0; // all lamps OFF
			else
				displayvalue = c->value;
			mcout_printf(&mcout, "%-*s (%2d bit %-7s)=%s%s%s", name_len, c->name, c->value_bitlen,
					blinkenlight_control_type_t_text(c->type),
					radix_u642str(displayvalue, c->radix, c->value_bitlen, 0),
					radix_getname_char(c->radix), //o, h, d
					c->value_previous != c->value ? " !" : "  ");
		}
	}
	mcout_flush(&mcout, stdout, panelsim_menu_linewidth, "  ||  ", /*first_col_then_row*/0);

	for (name_len = i_control = 0; i_control < panelsim_panel->controls_count; i_control++) {
		c = &(panelsim_panel->controls[i_control]);
		if (c->is_input && name_len < strlen(c->name))
			name_len = strlen(c->name);
	}
	printf("* Inputs:\n");
	mcout_init(&mcout, MAX_BLINKENLIGHT_PANEL_CONTROLS); // select numbers = sequential count of input controls
	for (i_incontrol = i_control = 0; i_control < panelsim_panel->controls_count; i_control++) {
		c = &(panelsim_panel->controls[i_control]);
		if (c->is_input) {
			uint64_t displayvalue;
			if (panelsim_panel->mode == RPC_PARAM_VALUE_PANEL_MODE_ALLTEST)
				// "selftest": show "all ones"
				// whatever "selftest on input switches" this means for a panel
				displayvalue = BitmaskFromLen64[c->value_bitlen];
			else
				displayvalue = c->value;
			mcout_printf(&mcout, "%2d) %-*s (%2d bit %-7s)=%s%s", i_incontrol, name_len, c->name,
					c->value_bitlen, blinkenlight_control_type_t_text(c->type),
					radix_u642str(displayvalue, c->radix, c->value_bitlen, 0),
					radix_getname_char(c->radix));
			i_incontrol++;
		}
	}
	mcout_flush(&mcout, stdout, panelsim_menu_linewidth, "  ||  ", /*first_col_then_row*/0);
}

/*
 *	userinput()
 *	process string entered by user
 */
void panelsim_userinput(char *user_input)
{
	unsigned i_control, i_incontrol;
	blinkenlight_control_t *c;

	if (!strcasecmp(user_input, "q")) {
		printf("\nExit on user request\n");
		exit(0);
	}
// parse <nr> [<value>]
	{
		uint64_t value;
		char valuebuffer[80];
		int n_fields = sscanf(user_input, "%d %s", &i_incontrol, valuebuffer);
		if (n_fields > 0) {
			// find again the "i_incontrol"th input control
			for (i_control = 0; i_control < panelsim_panel->controls_count; i_control++) {
				c = &(panelsim_panel->controls[i_control]);
				if (c->is_input) {
					if (i_incontrol == 0) // stop if down count to 0, then c valid
						break;
					i_incontrol--;
				}
				c = NULL;
			}
			if (c) {
				if (n_fields < 2) { // no <value> entered, require it
					printf("new %s value for output control %s (%d significant bits): ",
							radix_getname_short(c->radix), c->name, c->value_bitlen);
					scanf("%s", valuebuffer);
					printf("\n");
				}
				if (c->blinkenbus_register_wiring_count == 0)
					printf("const control, can not be changed\n");
				else {
					// interpret value according to radix of control
					if (!radix_str2u64(&value, c->radix, valuebuffer)) {
						fprintf(stderr, "Illegal %s value \"%s\" entered!\n",
								radix_getname_long(c->radix), valuebuffer);
					} else {
						// trunc value to valid bits
						value &= BitmaskFromLen64[c->value_bitlen];
						c->value = value;
						// input value now in global struct, can be transmitted to client on request
					}
				}
			}
		}

	}

}

/*
 *	service()
 *	called periodically by RPC server idle timeout
 */
void panelsim_service(void)
{
#define MSECS(tv) ((uint64_t)((tv).tv_sec)*1000 + (tv).tv_usec / 1000 )
	struct timeval current_time_tv;
	int curchr;

	// scan for key press, first char into buffer
	// !!! Works not in Eclipse condole !!!
//	set_conio_terminal_mode();
	curchr = os_kbhit();
	if (curchr) {
//		user_input_in_progress = 1;
		//curchr = getch();
		//if (curchr < 0)
		//{
		//	fprintf(stderr, "Error %d on input!\n", curchr);
		//}
		//else
		//{
		// if CR or buffer full: process input
		if (curchr == '\n' || user_input_chars + 1 >= sizeof(user_input_buffer)) {
			printf("Processing \"%s\"\n", user_input_buffer);
			user_input_buffer[user_input_chars] = '\0';
			panelsim_userinput(user_input_buffer);
			update_needed = 1; // panel state changed
			user_input_buffer[0] = '\0'; // empty buffer
			user_input_chars = 0;
		} else {
			// just save & echo input character
			user_input_buffer[user_input_chars++] = curchr;
			user_input_buffer[user_input_chars] = '\0'; // terminate
		}
		//}
	}
//	reset_terminal_mode();

	gettimeofday(&current_time_tv, NULL);

	// do not update more often then every MIN_UPDATE_INTERVALL_US usecs
	if ((MSECS(update_last_time_tv) + MIN_UPDATE_INTERVALL_MS < MSECS(current_time_tv))
			&& update_needed /*&& !user_input_in_progress*/) {
		update_last_time_tv = current_time_tv;
		update_needed = 0;
		panelsim_show();
		printf(" q)  quit\n");
		printf("\nSet input by entering \"<nr> <value>\"\n");
		// re-print current user input
		printf(">>> %s", user_input_buffer);
	}
}

/*
 * panelsim_write_panel_output_controls()
 * write all output control values of a panel to GUI
 * produces new printout (after timeout, and after
 * force_all: no optimization
 */
void panelsim_write_panel_output_controls(blinkenlight_panel_t *p, int force_all)
{
	print(LOG_DEBUG, "panelsim_write_panel_output_controls(panel=%s)\n", p->name);
	if (p != panelsim_panel) {
		print(LOG_DEBUG, "panel not selected for simulation\n");
		return; // act only on selected panel simualtion
	}
	// signal service(): new print out
	update_needed = 1;
}

/*
 * panelsim_read_panel_input_controls()
 * nothing happens, since the GUI already operates on the
 * global panelist[] struct.
 */
void panelsim_read_panel_input_controls(blinkenlight_panel_t *p)
{
	print(LOG_DEBUG, "panelsim_read_panel_input_controls(panel=%s)\n", p->name);

	if (p != panelsim_panel) {
		print(LOG_DEBUG, "panel not selected for simulation\n");
		return; // act only on selected panel simualtion
	}

}

/*
 * Simulation: 1, if all BlinkenBoards of the panel are enabled
 * Just a staic variable
 */
int panelsim_get_blinkenboards_state(blinkenlight_panel_t *p)
{
	return panelsim_panel_drivers_state;
}
void panelsim_set_blinkenboards_state(blinkenlight_panel_t *p, int new_state)
{
	panelsim_panel_drivers_state = new_state;
}

/*
 * init()
 * select ONE defined panel for simulation
 * i_panel = index in _panel_list.panels[]
 */
void panelsim_init(int i_panel)
{
	unsigned i_control;
	blinkenlight_control_t *c;

	//mcout_selftest() ;

	panelsim_panel = &(blinkenlight_panel_list->panels[i_panel]);

	for (i_control = 0; i_control < panelsim_panel->controls_count; i_control++) {
		c = &(panelsim_panel->controls[i_control]);

		// reset all values
		c->value = c->value_default;
		c->value_previous = c->value;
	}

	panelsim_panel_drivers_state = RPC_PARAM_VALUE_PANEL_BLINKENBOARDS_STATE_ACTIVE;

	// initialize all panel output controls with default values
	panelsim_write_panel_output_controls(panelsim_panel, /*force_all=*/1);

	update_last_time_tv.tv_sec = 0;
	update_last_time_tv.tv_usec = 0;

	//user_input_in_progress = 0;
	user_input_chars = 0;
	user_input_buffer[0] = '\0';

	update_needed = 1; // next service() shows the display
}

