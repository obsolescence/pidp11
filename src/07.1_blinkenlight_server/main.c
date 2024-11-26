/* main.c: RPC Server for BlinkenBus panel hardware interface.

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

 1-Apr-2016    JH  V 1.10 Low pass for output controls, major changes
 16-Mar-2016    JH  V 1.09 better commandline processing with getopt2()
 20-Feb-2016    JH  V 1.08 migration to VS2015 and repair
 08-Sep-2013    JH  V 1.07 Added check for duplicate panel names and duplicate control names
 01-Aug-2013    JH  V 1.06 Bugfix: board addr 0 was not allowed.
 22-Feb-2013    JH  V 1.05 On receive, control values are masked to valid bits.
 28-Jan-2013    JH  V 1.04 support for panel modes 1 and 2 for lamp and switch selftest
 30-Dec-2012    JH  V 1.03 Fixed back in blinkenbus.c, blinkenbus_read_panel_input_controls():
 range of used input registers was wrong calculated.
 07-Dec-2012    JH  V 1.02 Updated to handle the PDP-10 KI10 console:
 - now 100 controls allowed
 - Controls may have undefined lower bits
 (the KI10 PROGRAM COUNTER has only LEDs for Bits 35..18)
 08-Feb-2012    JH  created



 RPC Server for Blinkenlight panel hardware interface.

 To be run on a Beaglebone, must be compiled for ARM7 / Angstrom environment.

 Under WIN32, only operation is "test config file"

 Interfaces:
 1a) Interface to I/O boards
 - Wires of real blinkenlight panels are connected to
 I/O terminals on BLINKENBUS boards.
 - Voltages levels are set/read by accessing I/O registers on boards
 over BLINKENBUS
 - BLINKENBUS is a 8 bit address/ 8b it data bus, connected to BeagleBone GPIOs
 - kblinkenbus kernel module/driver maps IOBIS address space to file interface,
 so sets of consecutive BLINKENBUS addresses can be easily accessed with
 file read()/write()/lseek()/ioctl() calls
 1b) Simulated panel: Interactive interface to user
 in mode "panelsim",  panel controls are not realized by REAL blinkenlight console
 but over user interface.
 In this mode, the application still runs as server, but
 running as daemon makes no sense.

 2) Panel definitions
 Application talk to the blinkenlight console in terms of
 "panels" and "controls".
 - Many blinkenlight panels can be run in parallel on different IO boards
 - a panel cosists of many "controls": highlevel user interface components
 Example: a rotary switch with 4 positions, a row of 16 bit-switches for
 data word entry, a row of 22 LEDs for displaying a 22bit address, ....
 - A hierarchical configuration file ("blinkenlight panel config")
 defines panels, controls, and how controls are connected  to IOBIS I/O boards.
 - "one configuration file describes what is connected to one Beaglebone".
 3) Interface to applications over RPC
 - one server runs on one Beaglebone, but can serve multiple applications
 over a RPC (remote procedue call) API.
 - API function include
 "get control value", "set control value", "poll changed input controls"
 "get available panels" etc
 - a panel or control is identified by a "handle", which is defined in the
 configuration file.
*/ /*
 3) Output Lowpass Timing
   Output (Lamps) are low passes to prevent visual flicker.
   Critical for flicker are LEDs, light bulbs are slow on its own.
   A 3-stage pipeline is implemented
 3.0) Client:
   The Client (SimH) updates controls quite fast (1ms), but this does not prevent flicker.

 3.1) Averager:
   Each lamp has a lowpass cut-off frequency defined (control->fmax), the average value
   for that period is calculated. A LED is typically sampled for 10Hz = 100ms = 100 values from SimH
   Each LED has now an analogue brightness value 0..100%
   An analogue level resolution is given by client update interval and low pass interval
   Resolution = CUI / LPI. Example: SimH interval = 2ms, Lowpass = 100ms
   -> 50 samples per lowpass interval -> resolution of 2%.

 3.2)  Updater:
   the LED average values are periodically calculated by the "update" thread
   and transfered to the display stage. Update must be fast enough to not introduce flicker
   Recommended: 50Hz = 20ms.

 3.3) output multiplexer
   The hardware allows outputs only to e ON or OFF.
   Analoge values are generated by switching lets fast On and OFF.
   A "MUX" thread displays for each LED an circular pattern with "brightness" percent
   active phases. Cycle time for one pattern is perception frequency.
   Pattern length defines displayable brightness levels and needed update frequency.
   If cycle time = 50Hz= 20ms, and pattern length =32, then update frequency is
   20ms/32 =  625 us = 1.6kHz. BlineknBus signals are generated by a driver in software
   output for 3 Boards (KI10) was measured to be To be around 500us.
   To reduce the mux frequency a double frequency PWM pattern can be used, reducing the MUX
   frequency to 25 Hz -> 40ms/32 = 1250us mux period
   For pattern length = 16 (16 brightness levels) we have 40ms/16 = 2.5ms MUX cycle time

   MUX cycle time must be faster than the Updater period: to display an updated values
   at least one MUX cycle is necessary. At leaast: MUX cycle time < 2*updater period.

3.4) Current values
  SimH interval 1ms
  LED lowpass = 100ms,
  updater time 20ms = 50Hz
  MUX frequency = 1kHz = 50% CPU load on 700MHz BeageBone white
 */

#define MAIN_C_

#define VERSION	"v1.10"
#define COPYRIGHT_YEAR	2016

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>
#ifndef WIN32
#include <unistd.h>
#endif

#include "getopt2.h"
#include "print.h"
#include "blinkenlight_panels.h"
#include "blinkenlight_api_server_procs.h"
#include "config.h"
#include "panelsim.h"

#include "main.h"

#ifdef WIN32
#else
///// for Blinkenlight API server
#include "rpc_blinkenlight_api.h"
#include <rpc/pmap_clnt.h> // different name under "oncrpc for windows"?
#include "blinkenbus.h"
#include "iopattern.h"
#endif
//#include <memory.h>
//#include <sys/socket.h>
//#include <netinet/in.h>

#ifndef SIG_PF
#define SIG_PF void(*)(int)
#endif
///// end

//extern int stmode; // no server, just test config
static char program_info[1024];
static char program_name[1024]; // argv[0]
static char program_options[1024]; // argv[1.argc-1]

// command line args
getopt_t getopt_parser;

char configfilename[MAX_FILENAME_LEN];
static char logfilename[MAX_FILENAME_LEN];

// global flags
int mode_test; // no server, just test config
int mode_panelsim; // do not access BLINKENBUS, no daemon, user oeprates simualted panel

/*
 * print program info
 */
void info(void)
{
    print(LOG_INFO, "\n");
#ifdef WIN32
    print(LOG_NOTICE,"blinkenlightd %s - crippled WIN32 version of server for Blinkenlight hardware interface \n", VERSION);
#else
    print(LOG_NOTICE,
            "*** blinkenlightd %s - server for BeagleBone blinkenlight panel interface ***\n",
            VERSION);
#endif
    print(LOG_NOTICE, "    Compiled " __DATE__ " " __TIME__ "\n");
    print(LOG_NOTICE, "    Copyright (C) 2012-2016 Joerg Hoppe.\n");
    print(LOG_NOTICE, "    Contact: j_hoppe@t-online.de\n");
    print(LOG_NOTICE, "    Web: www.retrocmp.com/projects/blinkenbone\n");
    print(LOG_NOTICE, "\n");
}

/*
 * print help
 */
static void printheader()
{
#ifdef WIN32
    fprintf(stderr, "blinkenlightd %s - crippled WIN32 version of server for BlinkenBone hardware interface \n", VERSION);
#else
    fprintf(stderr, "blinkenlightd %s - RPC server for BlinkenBone hardware interface \n",
    VERSION);
#endif
    fprintf(stderr, "    Build " __DATE__ " " __TIME__ "\n");
    fprintf(stderr, "    Copyright (C) 2012-%d Joerg Hoppe.\n", COPYRIGHT_YEAR);
    fprintf(stderr, "    Contact: j_hoppe@t-online.de\n");
    fprintf(stderr, "    Web: www.retrocmp.com/projects/blinkenbone\n");
    fprintf(stderr, "\n");
}

static void help()
{
    fprintf(stdout, "Command line summary:\n\n");
    // getop must be intialized to print the syntax
    getopt_help(&getopt_parser, stdout, 80, 10, "blinkenlightapitst");
    exit(1);
}

// show error for one option
static void commandline_error()
{
    fprintf(stdout, "Error while parsing commandline:\n");
    fprintf(stdout, "  %s\n", getopt_parser.curerrortext);
    exit(1);
}
// parameter wrong for currently parsed option
static void commandline_option_error()
{
    fprintf(stdout, "Error while parsing commandline option:\n");
    fprintf(stdout, "  %s\nSyntax:  ", getopt_parser.curerrortext);
    getopt_help_option(&getopt_parser, stdout, 96, 10);
    exit(1);
}

static int parse_commandline(int argc, char **argv)
{
    int i;
    int opt_background = 0;
    int res;

    strcpy(program_name, argv[0]);
    strcpy(program_options, "");
    for (i = 1; i < argc; i++) {
        if (i > 1)
            strcat(program_options, " ");
        strcat(program_options, argv[i]);
    }

    // define commandline syntax
    getopt_init(&getopt_parser, /*ignore_case*/1);
    getopt_def(&getopt_parser, "?", "help", NULL, NULL, NULL, "Print help", NULL, NULL, NULL, NULL);
    getopt_def(&getopt_parser, "c", "config_file", "config_filename", NULL, NULL,
            "specify name of panel configuration file\n"
                    "Defines, how panels are connected to BLINKENBUS interface boards",
            "pdp1170.conf", "read control definitions for the BlinkenBone PDP-11/70 panel", NULL,
            NULL);
#ifndef WIN32
    getopt_def(&getopt_parser, "b", "background", NULL, NULL, NULL,
            "background operation: print to syslog (view with dmesg)\n"
                    "default output is stderr",
            NULL, NULL, NULL, NULL);
    getopt_def(&getopt_parser, "s", "simulation", NULL, NULL, NULL,
            "show user GUI with simulated panel.\n"
                    "No operation on BLINKENBUS. Clears -b, default output is stderr.",
            NULL, NULL, NULL, NULL);
    getopt_def(&getopt_parser, "v", "verbose", NULL, NULL, NULL, "tell what I'm doing",
    NULL, NULL, NULL, NULL);
#endif
    getopt_def(&getopt_parser, "t", "test", NULL, NULL, NULL,
            "go not into server mode, just test the config file",
            NULL, NULL, NULL, NULL);

    if (argc < 2)
        help(); // at least 1 required

    // clear vars
    strcpy(configfilename, "");
    mode_test = 0;
    mode_panelsim = 0;

    res = getopt_first(&getopt_parser, argc, argv);
    while (res > 0) {
        if (getopt_isoption(&getopt_parser, "help")) {
            help();
        } else if (getopt_isoption(&getopt_parser, "background")) {
            opt_background = 1;
        } else if (getopt_isoption(&getopt_parser, "config_file")) {
            if (getopt_arg_s(&getopt_parser, "config_filename", configfilename,
                    sizeof(configfilename)) < 0)
                commandline_option_error();
        } else if (getopt_isoption(&getopt_parser, "simulation")) {
            mode_panelsim = 1;
        } else if (getopt_isoption(&getopt_parser, "test")) {
            mode_test = 1;
        } else if (getopt_isoption(&getopt_parser, "verbose")) {
            print_level = LOG_DEBUG;
        }

        res = getopt_next(&getopt_parser);
    }
    if (res == GETOPT_STATUS_MINARGCOUNT || res == GETOPT_STATUS_MAXARGCOUNT)
        // known option, but wrong number of arguments
        commandline_option_error();
    else if (res < 0)
        commandline_error();

    // all arguments set?
    if (!strlen(configfilename)) {
        fprintf(stderr, "Missing mandatory arguments!\n");
        return 0;
    }
    if (mode_panelsim) {
        opt_background = 0; // interactive simulation
    }

    // start logging
    print_open(opt_background); // if background, then syslog

//     for (index = optind; index < argc; index++)
//       printf ("Non-option argument %s\n", argv[index]);
//     return 0;
    return 1;
}

#ifndef WIN32
/******************************************************
 * Server for Blinkenlight API
 * see code generated by rpcgen
 * DOES NEVER END!
 */
void blinkenlight_api_server(void)
{
    register SVCXPRT *transp;

// entry to server stub

    void blinkenlightd_1(struct svc_req *rqstp, register SVCXPRT *transp);

    pmap_unset(BLINKENLIGHTD, BLINKENLIGHTD_VERS);

    transp = svcudp_create(RPC_ANYSOCK);
    if (transp == NULL) {
        print(LOG_ERR, "%s", "cannot create udp service.");
        exit(1);
    }
    if (!svc_register(transp, BLINKENLIGHTD, BLINKENLIGHTD_VERS, blinkenlightd_1, IPPROTO_UDP)) {
        print(LOG_ERR, "%s", "unable to register (BLINKENLIGHTD, BLINKENLIGHTD_VERS, udp).");
        exit(1);
    }

    transp = svctcp_create(RPC_ANYSOCK, 0, 0);
    if (transp == NULL) {
        print(LOG_ERR, "%s", "cannot create tcp service.");
        exit(1);
    }
    if (!svc_register(transp, BLINKENLIGHTD, BLINKENLIGHTD_VERS, blinkenlightd_1, IPPROTO_TCP)) {
        print(LOG_ERR, "%s", "unable to register (BLINKENLIGHTD, BLINKENLIGHTD_VERS, tcp).");
        exit(1);
    }

    // svc_run();
    // alternate implementation of svn_run() with periodically timeout and
    // 	calling of callback
    {
        fd_set readfds;
        struct timeval tv;
        int dtbsz = getdtablesize();
        for (;;) {
            readfds = svc_fdset;
            tv.tv_sec = 0;
            tv.tv_usec = 1000 * 2; // every 10 ms*time_slice_ms;
            switch (select(dtbsz, &readfds, NULL, NULL, &tv)) {
            case -1:
                if (errno == EINTR)
                    continue;
                perror("select");
                return;
            case 0: // timeout
                    // provide the panel simulation with computing time
                if (mode_panelsim)
                    panelsim_service();
                break;
            default:
                svc_getreqset(&readfds);
                break;
            }
        }
    }

    print(LOG_ERR, "%s", "svc_run returned");
    exit(1);
    /* NOTREACHED */

}
#endif

// gets called when RPC client wants panel control values
static void on_blinkenlight_api_panel_get_controlvalues(blinkenlight_panel_t *p)
{
/// read values from BLINKENBUS into input controls values
    if (mode_panelsim)
        panelsim_read_panel_input_controls(p);
#ifndef WIN32
    else {
//        blinkenbus_map_t cache ;
//        blinkenbus_cache_from_blinkenboards_inputs(cache, p) ;
        blinkenbus_map_dump(blinkenbus_input_cache,
                "on_blinkenlight_api_panel_get_controlvalues()\n");
        // blinkenbus_input_cache updated by mux thread
        blinkenbus_inputcontrols_from_cache(blinkenbus_input_cache, p, /*raw*/FALSE);
    }
#endif
}

// gets called when RPC client updated panel control values
static void on_blinkenlight_api_panel_set_controlvalues(blinkenlight_panel_t *p, int force_all)
{
    /// write values of panel controls to BLINKENBUS. optimization: only changed
    if (mode_panelsim)
        panelsim_write_panel_output_controls(p, force_all);
#ifndef WIN32
    else {
        // if non-simulation mode, blinkenbus is interfaced by iopattern - threads
        // no operation here
        /*
         unsigned i_control ;
         unsigned phase ;
         blinkenlight_control_t *c;
         // TEST: set all cache phases to same value

         for (i_control = 0; i_control < p->controls_count; i_control++) {
         c = &(p->controls[i_control]);
         if (!c->is_input) {
         for (phase=0 ; phase < IOPATTERN_OUTPUT_PHASES ; phase++)
         // here testmode logic
         blinkenbus_outputcontrol_to_cache(blinkenbus_output_caches[phase], c, c->value >> phase) ;
         //               blinkenbus_outputcontrol_to_cache(blinkenbus_output_caches[phase], c, c->value) ;
         //  blinkenbus_outputcontrols_to_cache(blinkenbus_output_caches[phase], p) ;
         }
         }


         /*


         blinkenbus_map_t cache ;
         blinkenbus_cache_from_blinkenboards_outputs(cache) ;
         //      blinkenbus_map_dump(cache, "on_blinkenlight_api_panel_set_controlvalues:\nafter blinkenbus_cache_from_blinkenboards_outputs()\n") ;

         blinkenbus_outputcontrols_to_cache(cache, p) ;
         //        blinkenbus_map_dump(cache, "after blinkenbus_panel_to_cache()\n") ;

         blinkenbus_cache_to_blinkenboards_outputs(cache, force_all) ;
         */
    }
#endif
}

static int on_blinkenlight_api_panel_get_state(blinkenlight_panel_t *p)
{
// set tristate of all BlinkenBoards of this panel.
// side effect to other panls on the same boards!
    if (mode_panelsim)
        return panelsim_get_blinkenboards_state(p);
#ifdef WIN32
    else
    return 0;
#else
    else
        return blinkenbus_get_blinkenboards_state(p);
#endif
}

static void on_blinkenlight_api_panel_set_state(blinkenlight_panel_t *p, int new_state)
{
// set tristate of all BlinkenBoards of this panel.
// side effect to other panels on the same boards!
    if (mode_panelsim)
        panelsim_set_blinkenboards_state(p, new_state);
#ifndef WIN32
    else
        blinkenbus_set_blinkenboards_state(p, new_state);
#endif
}

static char *on_blinkenlight_api_get_info()
{
    static char buffer[1024];
#ifdef WIN32
	sprintf(buffer,
		"Server info  : %s\n"
		"Program name : %s\n"
		"Command line : %s\n"
		"Compile time : " __DATE__ " " __TIME__ "\n",
		program_info, program_name, program_options);
#else
    struct timespec ts;
    clock_getres(CLOCK_MONOTONIC, &ts);

    sprintf(buffer,
                    "Server info  : %s\n"
                    "Program name : %s\n"
                    "Command line : %s\n"
                    "Compile time : " __DATE__ " " __TIME__ "\n"
                    "Telemetrics  : Updater period = %u ms, MUX period = %u us;\n"
                    "               MUX pattern levels/phases = %u/%u; \n"
                    "               MUX cycle time = %u .. %u us, max %u%% CPU load.", //
            program_info, program_name, program_options,
            IOPATTERN_UPDATE_PERIOD_US/1000,
                 IOPATTERN_OUTPUT_MUX_PERIOD_US,
                 IOPATTERN_OUTPUT_BRIGHTNESS_LEVELS,
                 IOPATTERN_OUTPUT_PHASES,
            blinkenbus_min_cycle_time_ns/1000, blinkenbus_max_cycle_time_ns / 1000,
           (100*blinkenbus_max_cycle_time_ns) / (1000*IOPATTERN_OUTPUT_MUX_PERIOD_US)
            );
    blinkenbus_min_cycle_time_ns = blinkenbus_max_cycle_time_ns = 0; // new sample
#endif

    return buffer;
}

/*
 *
 */
int main(int argc, char *argv[])
{

    print_level = LOG_NOTICE;
    // print_level = LOG_DEBUG;
    if (!parse_commandline(argc, argv)) {
        help();
        return 1;
    }
    sprintf(program_info, "blinkenlightd - Blinkenlight API server daemon %s", VERSION);

    info();

    print(LOG_INFO, "Start\n");

    blinkenlight_panel_list = blinkenlight_panels_constructor();

    /*
     * load panel configuration
     */
    blinkenlight_panels_config_load(configfilename);

    blinkenlight_panels_config_fixup(blinkenlight_panel_list);

    // link set/get events
    blinkenlight_api_panel_get_controlvalues_evt = on_blinkenlight_api_panel_get_controlvalues;
    blinkenlight_api_panel_set_controlvalues_evt = on_blinkenlight_api_panel_set_controlvalues;
    blinkenlight_api_panel_get_state_evt = on_blinkenlight_api_panel_get_state;
    blinkenlight_api_panel_set_state_evt = on_blinkenlight_api_panel_set_state;
    blinkenlight_api_get_info_evt = on_blinkenlight_api_get_info;

    if (mode_test) {
        blinkenlight_panels_diagprint(blinkenlight_panel_list, stderr);

        if (!blinkenlight_panels_config_check()) {
            print(LOG_ERR, "Terminated with error.\n");
            exit(1);
        }
        print(LOG_INFO, "Terminated without error.\n");
    } else {
#ifdef WIN32
        print(LOG_ERR, "Under WIN32, only option \"-t: test config file\" is possible.\n");
#else
        if (mode_panelsim)
            panelsim_init(0); // at the moment, use first panel for simulation
        else
            iopattern_init();

        if (!blinkenlight_panels_config_check()) {
            print(LOG_ERR, "Terminated with error.\n");
            exit(1);
        }
        /*
         *  Enter API server mode
         */
        print(LOG_NOTICE, "Starting Blinkenlight API server\n");
        blinkenlight_api_server();
        // does never end!
#endif
    }

    print_close(); // well ....

    blinkenlight_panels_destructor(blinkenlight_panel_list);

    return 0;
}
