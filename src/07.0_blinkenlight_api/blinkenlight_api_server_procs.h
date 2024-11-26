/* blinkenlight_api_server_procs.h: Implementation of Blinkenlight API on the server

   Copyright (c) 2015-2016, Joerg Hoppe
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

   08-May-2016  JH      new event "set_controlvalue"
   22-Feb-2016	JH		added panel mode set/get callbacks
   13-Nov-2015  JH      created
 */

#ifndef BLINKENLIGHT_API_SERVER_PROCS_H_
#define BLINKENLIGHT_API_SERVER_PROCS_H_

#include "blinkenlight_panels.h"


		// type for panel control value set/get events
typedef void (*blinkenlight_api_panel_set_controlvalue_evt_t) (blinkenlight_panel_t *, blinkenlight_control_t *) ;
typedef void (*blinkenlight_api_panel_set_controlvalues_evt_t) (blinkenlight_panel_t *, /*force all*/int) ;
typedef void (*blinkenlight_api_panel_get_controlvalues_evt_t) (blinkenlight_panel_t *) ;
typedef int (*blinkenlight_api_panel_get_state_evt_t) (blinkenlight_panel_t *) ;
typedef void (*blinkenlight_api_panel_set_state_evt_t) (blinkenlight_panel_t *, int) ;
typedef int (*blinkenlight_api_panel_get_mode_evt_t) (blinkenlight_panel_t *) ;
typedef void (*blinkenlight_api_panel_set_mode_evt_t) (blinkenlight_panel_t *, int) ;
typedef char *(*blinkenlight_api_get_info_evt_t) (void) ;

#ifndef BLINKENLIGHT_API_SERVER_PROCS_C_

 // global panel config
extern blinkenlight_panel_list_t *blinkenlight_panel_list;


// pointer to callbacks
extern blinkenlight_api_panel_set_controlvalue_evt_t blinkenlight_api_panel_set_controlvalue_evt ;
extern blinkenlight_api_panel_set_controlvalues_evt_t blinkenlight_api_panel_set_controlvalues_evt ;
extern blinkenlight_api_panel_get_controlvalues_evt_t blinkenlight_api_panel_get_controlvalues_evt ;
extern blinkenlight_api_panel_get_state_evt_t blinkenlight_api_panel_get_state_evt ;
extern blinkenlight_api_panel_set_state_evt_t blinkenlight_api_panel_set_state_evt ;
extern blinkenlight_api_panel_get_mode_evt_t blinkenlight_api_panel_get_mode_evt ;
extern blinkenlight_api_panel_set_mode_evt_t blinkenlight_api_panel_set_mode_evt ;
extern blinkenlight_api_get_info_evt_t blinkenlight_api_get_info_evt ;
#endif

#endif /* BLINKENLIGHT_API_SERVER_PROCS_H_ */
