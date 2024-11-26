/* blinkenlight_api_client.h: Class for client-side of RPC Blinkenlight API

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


   16-Feb-2012  JH      created
 */


#ifndef BLINKENLIGHT_API_CLIENT_H_
#define BLINKENLIGHT_API_CLIENT_H_

//#include <rpc/rpc.h> /*  needed for CLIENT, but collision beteeen windows.h and SimH */

// compile with -DBLINKENLIGHT_CLIENT, for "panels"
#include "blinkenlight_panels.h"

#include "rpc_blinkenlight_api.h"

/*
 *	Error status
 *  0 = OK
 *	1 = error
 */
typedef int blinkenlight_api_status_t;

// class "client"
typedef struct
{

	// network name of RPC server host
	char *rpc_server_hostname;

	// client context for RPC. actual type is CLIENT.
	// untyped, because can not include rpc/rcp.h -> Collision SimH/<windows.h>
	void *rpc_client;

	// list of all panels published by server
	blinkenlight_panel_list_t *panel_list;

	char error_text[1024];
	char *error_file;
	int error_line;
	int connected; // 1 between connect() and disconnect()
} blinkenlight_api_client_t;

// create and destroy a client object.
blinkenlight_api_client_t *blinkenlight_api_client_constructor(void);
void blinkenlight_api_client_destructor(blinkenlight_api_client_t *_this);

// auxiliary: get last error text
char *blinkenlight_api_client_get_error_text(blinkenlight_api_client_t *_this);

// manage connection to server
blinkenlight_api_status_t blinkenlight_api_client_connect(blinkenlight_api_client_t *_this,
		char *host_servername);
blinkenlight_api_status_t blinkenlight_api_client_disconnect(blinkenlight_api_client_t *_this);
blinkenlight_api_status_t blinkenlight_api_client_get_serverinfo(blinkenlight_api_client_t *_this,
		char *buffer, int buffersize);

// query panels and their controls from the server
blinkenlight_api_status_t blinkenlight_api_client_get_controls(blinkenlight_api_client_t *_this,
		blinkenlight_panel_t *p);
blinkenlight_api_status_t blinkenlight_api_client_get_panels_and_controls(
		blinkenlight_api_client_t *_this);

// read new values for input controls from server
blinkenlight_api_status_t blinkenlight_api_client_get_inputcontrols_values(
		blinkenlight_api_client_t *_this, blinkenlight_panel_t *p);
// write changed values for output controls to the server
blinkenlight_api_status_t blinkenlight_api_client_set_outputcontrols_values(
		blinkenlight_api_client_t *_this, blinkenlight_panel_t *p);

// get a param of a bus, panel, control
blinkenlight_api_status_t blinkenlight_api_client_get_object_param(blinkenlight_api_client_t *_this,
		unsigned *param_value, unsigned object_class, unsigned object_handle, unsigned param_handle) ;
// set a param of a bus, panel, control
blinkenlight_api_status_t blinkenlight_api_client_set_object_param(blinkenlight_api_client_t *_this,
		unsigned object_class, unsigned object_handle, unsigned param_handle,
		unsigned param_value);

#endif /* BLINKENLIGHT_API_CLIENT_H_ */
