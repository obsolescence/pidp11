/* realcons_simh.c: Interface to SimH, "realcons" device.

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

   18-Jun-2016  JH      added param "bootimage" (for PDP-15)
   25-Feb-2016  JH      disconnect on host or panel change
   25-Mar-2012  JH      created
*/
#ifdef USE_REALCONS

#define REALCONS_SIMH_C_

#include "sim_defs.h"
#include "realcons.h"

/* Set/show data structures */

static CTAB set_realcons_tab[] =
{
{ "HOST", &realcons_simh_set_hostname, 0 },
{ "PANEL", &realcons_simh_set_panelname, 0 },
{ "INTERVAL", &realcons_simh_set_service_interval, 1 },
{ "CONNECTED", &realcons_simh_set_connect, 1 },
{ "DISCONNECTED", &realcons_simh_set_connect, 0 },
{ "BOOTIMAGE", &realcons_simh_set_boot_image, 0 },
{ "TEST", &realcons_simh_test, 0 },
{ "DEBUG", &realcons_simh_set_debug, 1 },
{ "NODEBUG", &realcons_simh_set_debug, 0 },

{ NULL, NULL, 0 } };

static SHTAB show_realcons_tab[] =
{
{ "HOST", &realcons_simh_show_hostname, 0 },
{ "PANEL", &realcons_simh_show_panelname, 0 },
{ "INTERVAL", &realcons_simh_show_service_interval, 0 },
//     { "LOG", &sim_show_log, 0 },
		{ "CONNECTED", &realcons_simh_show_connected, 0 },
        { "BOOTIMAGE", &realcons_simh_show_boot_image, 0 },
        { "DEBUG", &realcons_simh_show_debug, 0 },
		{ "SERVER", &realcons_simh_show_server, 0 }, // the last, multiline outout
//	{ "CYCLES", &realcons_simh_show_cycles, 0 }, // debug
		{ NULL, NULL, 0 } };

/*
 * set realcons host=<hostname>
 * set realcons panel=<panelname>
 * set realcons enabled
 * set realcons disabled
 * set realcons bootimage=<filename>
 * set realcons debug
 * set realcons nodebug

 */
t_stat sim_set_realcons(int32 flag, CONST char *cptr)
{
	char *cvptr, gbuf[CBUFSIZE];
	CTAB *ctptr;
	t_stat r;
	// generic code
	if ((cptr == NULL) || (*cptr == 0))
		return SCPE_2FARG;
	while (*cptr != 0) { /* do all mods */
		cptr = get_glyph_nc(cptr, gbuf, ','); /* get modifier */
		if ((cvptr = strchr(gbuf, '=')))
			*cvptr++ = 0; /* = value? */
		get_glyph(gbuf, gbuf, 0); /* modifier to UC */
		if ((ctptr = find_ctab(set_realcons_tab, gbuf))) { /* match? */
			r = ctptr->action(ctptr->arg, cvptr); /* do the rest */
			if (r != SCPE_OK)
				return r;
		} else
			return SCPE_NOPARAM;
	}
	return SCPE_OK;
}

// set realcons host=<name>
// (is initialized with "localhost")
// changes disconnect
t_stat realcons_simh_set_hostname(int32 flg, CONST char *cptr)
{
	if ((cptr == NULL) || (*cptr == 0))
		return SCPE_2FARG; /* too few arguments? */
	if (cpu_realcons->connected)
		realcons_disconnect(cpu_realcons);
	strcpy(cpu_realcons->application_server_hostname, cptr);
	return SCPE_OK;
}

// set realcons panel=<name>
t_stat realcons_simh_set_panelname(int32 flg, CONST char *cptr)
{
	if ((cptr == NULL) || (*cptr == 0))
		return SCPE_2FARG; /* too few arguments? */
	if (cpu_realcons->connected)
		realcons_disconnect(cpu_realcons);
	strcpy(cpu_realcons->application_panel_name, cptr);
	return SCPE_OK;
}

// set realcons bootimage=<filename>
t_stat realcons_simh_set_boot_image(int32 flg, CONST char *cptr)
{
    if ((cptr == NULL) || (*cptr == 0))
        return SCPE_2FARG; /* too few arguments? */
    strcpy(cpu_realcons->boot_image_filepath, cptr);
    return SCPE_OK;
}



/*
 * set min period between two service cycles: update frequency for GUI and servers
 */
t_stat realcons_simh_set_service_interval(int32 flg, CONST char *cptr)
{
	t_value val;
	const char *tptr;
	if ((cptr == NULL) || (*cptr == 0))
		return SCPE_2FARG; /* too few arguments? */
	val = strtotv(cptr, &tptr, 10);
	if (cptr == tptr)
		return SCPE_ARG;
	if (*tptr != 0)
		return SCPE_ARG;
	cpu_realcons->service_interval_msec = (unsigned)val;
	return SCPE_OK;
}

/*
 * set realcons connected / disconnected
 */
t_stat realcons_simh_set_connect(int32 flg, CONST char *cptr)
{
	t_stat reason;
	if (flg) // connect to host
		reason = realcons_connect(cpu_realcons, cpu_realcons->console_logic_name,
				cpu_realcons->application_server_hostname,
				cpu_realcons->application_panel_name);
	else
		// disconnect
		reason = realcons_disconnect(cpu_realcons);
	return reason;
}

t_stat realcons_simh_test(int32 flg, CONST char *cptr)
{
	realcons_test(cpu_realcons, 0); // panel specific self test. (1 sec lamp test)
	return SCPE_OK;
}

t_stat realcons_simh_set_debug(int32 flg, CONST char *cptr)
{
	cpu_realcons->debug = flg;
	return SCPE_OK;
}

/* SHOW realcons command */

t_stat sim_show_realcons(FILE *st, DEVICE *dptr, UNIT *uptr, int32 flag, CONST char *cptr)
{
	char gbuf[CBUFSIZE];
	SHTAB *shptr;
	int32 i;
	fprintf(st, "REALCONS %s", cpu_realcons->console_logic_name);
	if (*cptr == 0) { /* show all */
		for (i = 0; show_realcons_tab[i].name; i++) {
			fprintf(st, ", ");
			show_realcons_tab[i].action(st, dptr, uptr, show_realcons_tab[i].arg, cptr);
		}
		fprintf(st, "\n");
		return SCPE_OK;
	}
	while (*cptr != 0) {
		cptr = get_glyph(cptr, gbuf, ','); /* get modifier */
		if ((shptr = find_shtab(show_realcons_tab, gbuf))) {
			fprintf(st, ", ");
			shptr->action(st, dptr, uptr, shptr->arg, cptr);
		} else
			return SCPE_NOPARAM;
	}

	fprintf(st, "\n");
	return SCPE_OK;
}

t_stat realcons_simh_show_hostname(FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag, CONST char *cptr)
{
	if (cptr && (*cptr != 0))
		return SCPE_2MARG;
	if (!cpu_realcons->application_server_hostname[0])
		fprintf(st, "no host");
	else
		fprintf(st, "host=\"%s\"", cpu_realcons->application_server_hostname);
	return SCPE_OK;
}

t_stat realcons_simh_show_panelname(FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag,
    CONST char *cptr)
{
	if (cptr && (*cptr != 0))
		return SCPE_2MARG;
	if (!cpu_realcons->application_panel_name[0])
		fprintf(st, "host panel not set");
	else
		fprintf(st, "host panel=\"%s\"", cpu_realcons->application_panel_name);
	return SCPE_OK;
}

t_stat realcons_simh_show_boot_image(FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag,
    CONST char *cptr)
{
    if (cptr && (*cptr != 0))
        return SCPE_2MARG;
    if (strlen(cpu_realcons->boot_image_filepath) == 0)
        fprintf(st, "boot image filename not set");
    else
        fprintf(st, "boot image filename=\"%s\"", cpu_realcons->boot_image_filepath);
    return SCPE_OK;
}


t_stat realcons_simh_show_service_interval(FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag,
    CONST char *cptr)
{
	if (cptr && (*cptr != 0))
		return SCPE_2MARG;
	fprintf(st, "interval = %u msec", cpu_realcons->service_interval_msec);
	return SCPE_OK;
}

// if connected, request info from blinkenlight API server
t_stat realcons_simh_show_server(FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag, CONST char *cptr)
{
	char buffer[1024];
	unsigned blinkenboards_state;
	if (cpu_realcons->connected) {
		if (strlen(cpu_realcons->console_model->info))
			fprintf(st, "panel info=\n\"%s\",\n", cpu_realcons->console_model->info);

		if (blinkenlight_api_client_get_object_param(cpu_realcons->blinkenlight_api_client,
				&blinkenboards_state, RPC_PARAM_CLASS_PANEL, cpu_realcons->console_model->index,
				RPC_PARAM_HANDLE_PANEL_BLINKENBOARDS_STATE) != RPC_ERR_OK)
			fprintf(st, "\nERROR detecting state of BlinkenBoards,");
		else
			fprintf(st, "\npanel BlinkenBoards driver hardware state = %s,",
					blinkenboards_state == RPC_PARAM_VALUE_PANEL_BLINKENBOARDS_STATE_ACTIVE ?
							"ACTIVE" :
							(blinkenboards_state
									== RPC_PARAM_VALUE_PANEL_BLINKENBOARDS_STATE_TRISTATE ?
									"TRISTATE" : "OFF"));
		blinkenlight_api_client_get_serverinfo(cpu_realcons->blinkenlight_api_client, buffer,
				sizeof(buffer));
		fprintf(st, "\nserver info=\n%s", buffer);
	} else
		fprintf(st, "no server");
	return SCPE_OK;
}

t_stat realcons_simh_show_connected(FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag,
    CONST char *cptr)
{
	if (cptr && (*cptr != 0))
		return SCPE_2MARG;
	if (cpu_realcons->connected)
		fprintf(st, "connected");
	else
		fprintf(st, "disconnected");
	return SCPE_OK;
}

t_stat realcons_simh_show_debug(FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag, CONST char *cptr)
{
	if (cptr && (*cptr != 0))
		return SCPE_2MARG;
	if (cpu_realcons->debug)
		fputs("debug", st);
	else
		fputs("nodebug", st);
	return SCPE_OK;
}

t_stat realcons_simh_show_cycles(FILE *st, DEVICE *dunused, UNIT *uunused, int32 flag, CONST char *cptr)
{
	if (cptr && (*cptr != 0))
		return SCPE_2MARG;
	fprintf(st, "%lld", cpu_realcons->service_cycle_count);
	return SCPE_OK;
}

#endif // USE_REALCONS
