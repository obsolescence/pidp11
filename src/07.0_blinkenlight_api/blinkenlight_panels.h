/* blinkenlight_panels.h: Blinkenlight API data structs panellist- panel - control

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

 10-Sep-2016  JH      added value_raw, as stage before input filtering
 25-May-2016  JH      added mux_code to register_wiring (control slice)
 22-Mar-2016  JH      allow non-BlinkenBoard hardware registers
 23-Feb-2016  JH      coding of const dummy input controls defined
 02-Feb-2012  JH      created


 Representation of a blinkenlight panel, on a level of "controls" and "states"
 (not "BLINKENBUS register bits")

 Is used by client and server, but with different fields
 You must define BLINKENLIGHT_SERVER or BLINKENLIGHT_CLIENT

 including mapping to BLINKENBUS register bits for access

 */

#ifndef BLINKENLIGHT_PANELS_H_
#define BLINKENLIGHT_PANELS_H_

#if !defined(BLINKENLIGHT_SERVER) && !defined(BLINKENLIGHT_CLIENT)
#error You must #define BLINKENLIGHT_SERVER or BLINKENLIGHT_CLIENT!
#endif

#include <stdio.h>	// FILE
#include <stdint.h>

#include "historybuffer.h"

// !!! keep #defines and structs in sync with blinkenlight_api.x !!!

// for simplicity, all lists are static arrays
#define MAX_BLINKENLIGHT_NAME_LEN 80
#define MAX_BLINKENLIGHT_INFO_LEN	1024
#define MAX_BLINKENLIGHT_PANELS	3
#define MAX_BLINKENLIGHT_PANEL_CONTROLS	200	// the PDP-10 KI10 has > 100 !
#define MAX_BLINKENLIGHT_REGISTERS_PER_CONTROL 8 //on panel control maybe spread accross max 8 BLINKENBUS registers
#define MAX_BLINKENLIGHT_HISTORY_ENTRIES	256 // worst case: 1ms update from client, 1/4 sec low pass -> must hold 250 entries
//#define MAX_BLINKENLIGHT_HISTORY_ENTRIES	16 // test

// Calculation of struct size:
// ( sizeof(blinkenlight_control_blinkenbus_register_wiring_struct) * MAX_BLINKENLIGHT_REGISTERS_PER_CONTROL
//  + MAX_BLINKENLIGHT_CONTROL_NAME_LEN )* MAX_BLINKENLIGHT_PANEL_CONTROLS
// (8 * 8 +80) *20 => 2.8 K
// And then many panels: 3 *2.8K = <10K

// a control is
// 1) a simple ON/OFF switch
// 2) or a input with multiple states (22bit switch row on 11/70, rotary switch , ...
// 3) or a single lamp (LED)
// 4) or a complex optical indicator (22 bit lamp row for "DATA" or "ADDRESS"
// 5) or a push button (momentary action)
// may also be an analog input or output or gauge or ....
enum blinkenlight_control_type_enum
{
	input_switch = 1, // SWITCH
	output_lamp = 2, // LAMP
	input_knob = 3, // KNOB
	output_pointer_instrument = 4, // POINTER
	input_other = 5, // generic INPUT
	output_other = 6 // generic OUTPUT
};

typedef enum blinkenlight_control_type_enum blinkenlight_control_type_t;

#define BLINKENLIGHT_IS_OUTPUT_CONTROL(type)	( \
		(type) == output_lamp \
		|| (type) == output_pointer_instrument \
		|| (type) == output_other\
		)

char *blinkenlight_control_type_t_text(blinkenlight_control_type_t x);

enum blinkenlight_register_space_enum
{
	input_register = 1, output_register = 0
};

typedef enum blinkenlight_register_space_enum blinkenlight_register_space_t;
char *blinkenlight_register_space_t_text(blinkenlight_register_space_t x);

#ifdef BLINKENLIGHT_SERVER
// Mapping between wires connected to BLINKENBUS registers
//	and bits for the value of a control.
// BlinkenBus has 512 input/output registers, each with 8 bit width
// Example: control_value_bit_offset = 8, blinkenbus_bitmask = 01100100:
//		bit 8 for the control signal value is assigned to register bit 2
//		bit 9   "   "   "         "     "   "    "     "  register bit 5
//		bit 10  "   "   "         "     "   "    "     "  register bit 6
//
// If other hardware is used (PiDP), hardware registers are not grouped by
//  "boards", and width of a register may be up to 32 sizeof(unsigned)
//
// If the panel lamps/switches are arranged in a multiplexing matrix (PDP-15)
// additional a "mux_code" must be defined.
// A control can consist of several slices (each a "wiring") accessed
// over different multiplexing rows.
typedef struct blinkenlight_control_blinkenbus_register_wiring_struct
{
	unsigned index; // index of this record in parent list
	/////  these members are load from config
	unsigned short control_value_bit_offset;// lowest bit of value defined here
	unsigned short blinkenbus_board_address;// boards 0..29 blinkenbus address
	unsigned short board_register_address;// register 0..15 on a board
	blinkenlight_register_space_t board_register_space;// input, output
	unsigned char blinkenbus_lsb;// lowest bit no of blinkenbus register
	// lowest bit on blinkenbus is lowest bit in value
	unsigned char blinkenbus_msb;// highest bit no of blinkenbus register
	int blinkenbus_levels_active_low;// 1, if bit levels are LOW active (key word "LEVELS = ACTIVE_HIGH | ACTIVE_LOW)

    //// if the panel has several rows selected by a multiplexer port:
    // value to access this "wiring". No generic function, must be interpreted by actual server application.
	unsigned   mux_code ;

	/////  these members are calculated:
	int blinkenbus_reversed;// 1, if bits between lsb and msb are to be reversed
	// then highest bit in blinkenbus is lowest bit in value,
	// lsb bit maps to  (lsb-msb+value_bit_offset)
	unsigned blinkenbus_register_address;// absolute register address in blinkenbus space, from board and local addr
	unsigned blinkenbus_bitmask_len;
	unsigned blinkenbus_bitmask;// mask with bitmask_len bits from blinkenbus register
	// if in blinkenbus_bitmask <n> bits are set,
	// this struct defines the value bits <offset>:<offset+n-1>


} blinkenlight_control_blinkenbus_register_wiring_t;

typedef enum blinkenlight_control_value_encoding_enum
{
	// "binary": bit pattern from BlinkenBus is interpreted as binary number
	// example: "00010100" -> value=12
	binary = 1,
	// "bitposition": bit pattern may only contain one bit set, value is the bit number
	// example "00000100" -> value = 2 (bit #2 set)
	bitposition = 2
}blinkenlight_control_value_encoding_t;
char *blinkenlight_control_value_encoding_t_text(blinkenlight_control_value_encoding_t x);
#endif


struct blinkenlight_panel_struct ;
typedef struct blinkenlight_control_struct
{
    struct blinkenlight_panel_struct *panel ; // uplink to parent
	unsigned index; // index of this record in control list of parent panel
	char name[MAX_BLINKENLIGHT_NAME_LEN];
	uint64_t tag; // application marker
	unsigned char is_input; // 0 = out, 1 = in
	blinkenlight_control_type_t type;
	uint64_t value; // 64bit: for instance for the LED row of a PDP-10 register (36 bit)
	uint64_t value_previous; // "old" value before change, for free use by client/server applications
        uint64_t value_default; // startup value
	unsigned radix; // number representation: 8 (octal) or 16 (hex)?
	unsigned value_bitlen; // relevant lsb's in value
	unsigned value_bytelen; // len of value in bytes ... for RPC transmissions
	//@	unsigned mode ; // 0 = normal, 1 = selftest (lamp test)
#ifdef BLINKENLIGHT_SERVER
	// the value for a control comes over IO boards attached to the BLINKENBUS.
	uint64_t value_raw ; // if history debouncing is used: as read from hardware

	// 1) define ordered list of wires, lowest value first
	unsigned blinkenbus_register_wiring_count;// count of blinkenbus registers carrying the control value
	// if "0", the input control is a dummy with constant value, "bitlen" must be set in config file
	blinkenlight_control_blinkenbus_register_wiring_t blinkenbus_register_wiring[MAX_BLINKENLIGHT_REGISTERS_PER_CONTROL];

	// 2) define coding
	// binary coded "0100" -> value=4
	blinkenlight_control_value_encoding_t encoding;

	// value from blinkenbus must be mirror bits:
	// bit [0] -> bit[bitlen-1], bit [1] -> bit[bitlen-2], ...
	unsigned mirrored_bit_order;

	unsigned	fmax ; // control can change max with that frequency, 0 = undefd.
	// Used in call of historybuffer_get_average_vals(..., 1000000/fmax,..)

	historybuffer_t *history;// ringbuffer for recent values
	// for each bit the average als value 0..255
	// calculated by historybuffer_get_average_vals(...bitmode=1)
	uint8_t averaged_value_bits[64];
	// arithmetic average of whole value
	// calculated by historybuffer_get_average_vals(...bitmode=0)
	uint64_t averaged_value;

#endif
} blinkenlight_control_t;

// a blinkenlight panel is a set of controls
typedef struct blinkenlight_panel_struct
{
	unsigned index; // index of this record in parent list
	char name[MAX_BLINKENLIGHT_NAME_LEN];
	// info is not transmitted over RPC, until Java/RemoteTea problem is solved !
	char info[MAX_BLINKENLIGHT_INFO_LEN];
	uint64_t tag; // application marker
	unsigned default_radix; // default number representation for controls: 8 (octal) or 16 (hex)?
	unsigned controls_count;
	blinkenlight_control_t controls[MAX_BLINKENLIGHT_PANEL_CONTROLS];
	unsigned controls_inputs_count; // separate count of inputs and outputs.(auxilliary)
	unsigned controls_outputs_count;
	// sum of bytes for values of all input/output controls
	// needed for compressed transmission of alle values over RPC byte stream
	unsigned controls_inputs_values_bytecount;
	unsigned controls_outputs_values_bytecount;

	// working mode
	// 0 = normal (RPC_PARAM_VALUE_PANEL_MODE_NORMAL)
	// 0x01 = historic accurate lamp test (RPC_PARAM_VALUE_PANEL_MODE_LAMPTEST)
	// 0x02 = test every control, inputs and outputs, (RPC_PARAM_VALUE_PANEL_MODE_ALLTEST)
	// 0x03 = "powerless": all controls dark, power button OFF, but still responsive to API
	unsigned mode;

} blinkenlight_panel_t;

// many panels can be connected to one BLINKENBUS,
// the list of panels is described here.
typedef struct blinkenlight_panel_list_struct
{
	unsigned panels_count;
	blinkenlight_panel_t panels[MAX_BLINKENLIGHT_PANELS];
} blinkenlight_panel_list_t;

blinkenlight_panel_list_t * blinkenlight_panels_constructor(void);
void blinkenlight_panels_destructor(blinkenlight_panel_list_t *_this);

void blinkenlight_panels_clear(blinkenlight_panel_list_t *_this);
blinkenlight_panel_t *blinkenlight_add_panel(blinkenlight_panel_list_t *_this);
blinkenlight_control_t *blinkenlight_add_control(blinkenlight_panel_list_t *_this,
		blinkenlight_panel_t *);
#ifdef BLINKENLIGHT_SERVER
blinkenlight_control_blinkenbus_register_wiring_t *blinkenlight_add_register_wiring(blinkenlight_control_t *c) ;
#endif

blinkenlight_panel_t * blinkenlight_panels_get_panel_by_name(blinkenlight_panel_list_t *_this,
		char *panelname);
blinkenlight_control_t * blinkenlight_panels_get_control_by_name(blinkenlight_panel_list_t *_this,
		blinkenlight_panel_t *p, char *controlname, int is_input);
unsigned blinkenlight_panels_get_control_value_changes(blinkenlight_panel_list_t *_this,
		blinkenlight_panel_t *p, int is_input);
unsigned blinkenlight_panels_get_max_control_name_len(blinkenlight_panel_list_t *_this,
		blinkenlight_panel_t *p);
#ifdef BLINKENLIGHT_SERVER
void blinkenlight_panels_config_fixup(blinkenlight_panel_list_t *_this) ;
#endif

void blinkenlight_panels_diagprint(blinkenlight_panel_list_t *_this, FILE *f);

#endif /* BLINKENLIGHT_PANELS_H_ */
