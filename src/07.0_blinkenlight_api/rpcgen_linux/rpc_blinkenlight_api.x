/* blinkenlight_api.x: Remote blinkenlight_api protocol XDR for rpcgen

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


   20-Feb-2016  JH      added PANEL_MODE_POWERLESS
   12-Feb-2012  JH      created
*/



/************* duplicated common definitions ***********/
/* const and structs as in  blinkenlight_panels.h
 * Keep in sync !!!
 */
const RPC_BLINKENLIGHT_API_MAX_NAME_LEN = 80; /* maximum length of a panel/control name */
typedef string rpc_blinkenlight_api_nametype<RPC_BLINKENLIGHT_API_MAX_NAME_LEN>; /* a panel/control name */

const RPC_BLINKENLIGHT_API_MAX_INFOSTRING_LEN = 1024;
typedef string rpc_blinkenlight_api_infostringtype<RPC_BLINKENLIGHT_API_MAX_INFOSTRING_LEN>; /* a multi line string */


enum rpc_blinkenlight_api_control_type_enum
{
	rpc_blinkenlight_api_input_switch = 1,
	rpc_blinkenlight_api_output_lamp = 2
};
typedef enum rpc_blinkenlight_api_control_type_enum rpc_blinkenlight_api_control_type_t ;

struct rpc_blinkenlight_api_panel_struct {
	rpc_blinkenlight_api_nametype name;
	/*!! with "Info, java/remotetea server do not work! */
	/*rpc_blinkenlight_api_infostringtype	info ; multi line string */
	unsigned controls_inputs_count; /* separate count of inputs and outputs */
	unsigned controls_outputs_count;
	/* sum of bytes for values of all input/output controls
	 * needed for compressed transmission of alle values over RPC byte stream*/

	unsigned controls_inputs_values_bytecount ;
	unsigned controls_outputs_values_bytecount ;
};

struct rpc_blinkenlight_api_control_struct {
	rpc_blinkenlight_api_nametype name;
    unsigned char   is_input ; /* 0 = out, 1 = in */
	rpc_blinkenlight_api_control_type_t type;
	int radix ;
	int value_bitlen ;
	int value_bytelen ; /* count for value transmission */
};


/* An array of control values.
 * for each value only "control->value_bytelen" bytes are transmitted, lsb first
 * so a 1 bit switchis transmitted as 1 byte, not as 8 bytes  */
struct rpc_blinkenlight_api_controlvalues_struct {
	int	error_code ; /* if used as result*/
	unsigned char	value_bytes<> ; /* dyn len */
} ;


/************** end duplicate definitions *************/

/*
 * The result of a GETINFO operation.
 */
struct rpc_blinkenlight_api_getinfo_res {
	int error_code ; /* 0 = OK */
	rpc_blinkenlight_api_infostringtype	info ; /* multi line string */
} ;

/*
 * The result of a GETPANELINFO operation.
 */
struct rpc_blinkenlight_api_getpanelinfo_res {
	int error_code ; /* 0 = OK */
	rpc_blinkenlight_api_panel_struct panel; /* no error: return panel */
} ;

/*
 * The result of a GETCONTROLINFO operation.
 */
struct rpc_blinkenlight_api_getcontrolinfo_res {
	int error_code ; /* 0 = OK */
	rpc_blinkenlight_api_control_struct control; /* no error: return control */
} ;


/*
 * The result of a SETPANEL_CONTROLVALUES operation.
 */
struct rpc_blinkenlight_api_setpanel_controlvalues_res {
	int error_code ; /* 0 = OK */
} ;

    /*
     * generic parameter get/set
     */
     const RPC_ERR_OK = 0 ;
     const RPC_ERR_PARAM_ILL_CLASS = 1 ;
     const RPC_ERR_PARAM_ILL_OBJECT = 2 ;
     const RPC_ERR_PARAM_ILL_PARAM = 3 ;
const RPC_PARAM_CLASS_BUS = 1;
const RPC_PARAM_CLASS_PANEL = 2;
const RPC_PARAM_CLASS_CONTROL = 3;
const RPC_PARAM_HANDLE_PANEL_BLINKENBOARDS_STATE= 1; /* 0 = driver pins on BlinkenBoard floating, 1 = driving*/
const RPC_PARAM_VALUE_PANEL_BLINKENBOARDS_STATE_OFF = 0 ;	/* at least one BlinkenBoard of panel not found on bus */
const RPC_PARAM_VALUE_PANEL_BLINKENBOARDS_STATE_TRISTATE = 1 ;	/* at least one BlinkenBoard of panel disabled/tristate */
const RPC_PARAM_VALUE_PANEL_BLINKENBOARDS_STATE_ACTIVE	= 2 ;	/* all BlinkenBoards of panel active */
const RPC_PARAM_HANDLE_PANEL_MODE = 2 ; /* Mode:0 = normal, 0x01 = lamp test, 0x2 = switch test (switches active on simulation), 0x3 = both */
const RPC_PARAM_VALUE_PANEL_MODE_NORMAL	= 0 ; /* panel in normal operating mode */
const RPC_PARAM_VALUE_PANEL_MODE_LAMPTEST = 1 ; /* historic accurate, test lamps */
const RPC_PARAM_VALUE_PANEL_MODE_ALLTEST = 2 ; /* test every control, inputs and outputs */
const RPC_PARAM_VALUE_PANEL_MODE_POWERLESS = 3 ; /* all lamps off, power button released, pnael over PAI repsonsive */

/* cmd to server: get a parameter value */
struct rpc_param_cmd_get_struct {
	unsigned	object_class ; /*RPC_PARAM_CLASS_*: bus, panel, control?*/
	unsigned	object_handle ;
	unsigned	param_handle ; /* RPC_PARAM_HANDLE_* */
} ;

/* parameter value from server */
struct   rpc_param_result_struct {
	int error_code ; /* 0 = OK, 1= class not found, 2= object not found, 3 = param not found */
	unsigned	object_class ; /* RPC_PARAM_CLASS_*: bus, panel, control?*/
	unsigned	object_handle ;
	unsigned	param_handle ; /* RPC_PARAM_HANDLE_* */
	unsigned	param_value ;
} ;

/* cmd to server: set parameter value */
struct rpc_param_cmd_set_struct {
	unsigned	object_class ; /*RPC_PARAM_CLASS_*: bus, panel, control?*/
	unsigned	object_handle ;
	unsigned	param_handle ; /* RPC_PARAM_HANDLE_* */
	unsigned	param_value ;
} ;

/*
 *	Test transfers
 */

/* send: byte count to request from server, */
/* result: byte count received by server, */
struct rpc_test_cmdstatus_struct {
	int	bytecount ;
} ;

/* data to send/receive from server */
struct rpc_test_data_struct {
	int	fixdata1 ;
	int	fixdata2 ;
	unsigned char vardata<> ; /* open array */
} ;


/* assigned program numbers
 *	http://www.iana.org/assignments/rpc-program-numbers/rpc-program-numbers.xml
 */
program BLINKENLIGHTD {
  version BLINKENLIGHTD_VERS {

    /* blinkenlight api */
    rpc_blinkenlight_api_getinfo_res RPC_BLINKENLIGHT_API_GETINFO(void) = 1;
    rpc_blinkenlight_api_getpanelinfo_res RPC_BLINKENLIGHT_API_GETPANELINFO(unsigned /*hPanel*/) = 2;
    rpc_blinkenlight_api_getcontrolinfo_res RPC_BLINKENLIGHT_API_GETCONTROLINFO(unsigned /*hPanel*/, unsigned /*hControl*/) = 3;
    rpc_blinkenlight_api_setpanel_controlvalues_res RPC_BLINKENLIGHT_API_SETPANEL_CONTROLVALUES(unsigned /*hPanel*/, rpc_blinkenlight_api_controlvalues_struct valuelist) = 4;
    rpc_blinkenlight_api_controlvalues_struct RPC_BLINKENLIGHT_API_GETPANEL_CONTROLVALUES(unsigned /*hPanel*/) = 5;
    /* generic parameter get/set */
    rpc_param_result_struct RPC_PARAM_GET(rpc_param_cmd_get_struct cmd_get) = 100;
    rpc_param_result_struct RPC_PARAM_SET(rpc_param_cmd_set_struct cmd_set) = 101;
    /* for performance tests */
    rpc_test_cmdstatus_struct RPC_TEST_DATA_TO_SERVER(rpc_test_data_struct data) = 1000;
    rpc_test_data_struct RPC_TEST_DATA_FROM_SERVER(rpc_test_cmdstatus_struct data) = 1001;

  } = 1;
} = 99;
