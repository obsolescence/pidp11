/* blinkenbus.h: Access to BlinkenBus registers

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

 10-Sep-2016    JH      added "raw" in blinkenbus_cache_from_blinkenboards_inputs()
                        and blinkenbuse_outputcontrols_to_cache()
   17-Feb-2012  JH      created
*/


#ifndef BLINKENBUS_H_
#define BLINKENBUS_H_

#include <stdio.h>
#include "rpc_blinkenlight_api.h" // param value const
#include "blinkenlight_panels.h"

/**** these are duplicates from the kblinkenbus driver sources ***/
#define DEVICE_FILE_NAME "blinkenbus" // should be used in /dev/
#define BLINKENBUS_MAX_BOARD_ADDR	0x1f	// valid boards adresses are 0 to 31
#define BLINKENBUS_MAX_REGISTER_ADDR	0x1ff	// 9 bit BLINKENBUS addresses

#define BLINKENBUS_ADDRESS_IO(board,reg) (((board) << 4) | ((reg) & 0xf))	// get adr of io registers
#define BLINKENBUS_ADDRESS_CONTROL(board)	BLINKENBUS_ADDRESS_IO(board,0xf)	// reg 15 is control reg

#define BLINKENBUS_INPUT_LOWPASS_F_CUTOFF 20000 // input filtershave low pass for 20kHz


#ifndef BLINKENBUS_C_
extern int blinkenbus_fd; // file descriptor for interface to driver
#endif

// copy of all registers on a blinkenbus
typedef unsigned char blinkenbus_map_t[BLINKENBUS_MAX_REGISTER_ADDR + 1];

void blinkenbus_init(void);

unsigned char blinkenbus_register_read(unsigned board_addr, unsigned reg_addr) ;
void blinkenbus_register_write(unsigned board_addr, unsigned reg_addr, unsigned char regval) ;

unsigned char blinkenbus_board_control_read(unsigned board_addr) ;
void blinkenbus_board_control_write(unsigned board_addr, unsigned char regval) ;

void blinkenbus_outputcontrol_to_cache(unsigned char *blinkenbus_cache, blinkenlight_control_t *c,
        uint64_t value);
void blinkenbus_inputcontrol_from_cache(unsigned char *blinkenbus_cache, blinkenlight_control_t *c, int raw) ;
void blinkenbus_outputcontrols_to_cache(unsigned char *blinkenbus_cache, blinkenlight_panel_t *p, int raw) ;
void blinkenbus_inputcontrols_from_cache(unsigned char *blinkenbus_cache, blinkenlight_panel_t *p, int raw) ;

void blinkenbus_cache_from_blinkenboards_outputs(unsigned char *blinkenbus_cache) ;
void blinkenbus_cache_to_blinkenboards_outputs(unsigned char *blinkenbus_cache, int force_all) ;
void blinkenbus_cache_from_blinkenboards_inputs(unsigned char *blinkenbus_cache) ;

void blinkenbus_set_blinkenboards_state(blinkenlight_panel_t *p, int new_state) ;
int blinkenbus_get_blinkenboards_state(blinkenlight_panel_t *p) ;

void blinkenbus_map_dump(unsigned char *map, char *info) ;


#endif /* BLINKENBUS_H_ */
