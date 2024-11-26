/* blinkenlight_panels.c: Blinkenlight API data structs panellist- panel - control

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
    22-Mar-2016  JH      allow non-BlinkenBoard hardware registers. _fixup() moved here.
    02-Feb-2012  JH      created
*/


#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "bitcalc.h"

#include "blinkenlight_panels.h"
#include "historybuffer.h"

#ifdef WIN32
#define strcasecmp _stricmp	// grmbl
#define strncasecmp _strnicmp
#endif

// access to latest node while parsing config file

/*
 * convert enums to text
 */
char *blinkenlight_control_type_t_text(blinkenlight_control_type_t x)
{
	switch (x) {
	case input_switch:
		return "SWITCH";
	case output_lamp:
		return "LAMP";
	case input_knob:
		return "KNOB";
	case output_pointer_instrument:
		return "POINTER";
	case input_other:
		return "INPUT";
	case output_other:
		return "OUTPUT";
	}
	return "???";
}

char *blinkenlight_register_space_t_text(blinkenlight_register_space_t x)
{
	switch (x) {
	case input_register:
		return "INPUT";
	case output_register:
		return "OUTPUT";
	}
	return "???";
}

#ifdef BLINKENLIGHT_SERVER

char *blinkenlight_control_value_encoding_t_text(blinkenlight_control_value_encoding_t x)
{
	static char buffer[80];
	switch(x)
	{
		case binary: return "BINARY";
		case bitposition: return "BITPOSITION";
	}
	return "???";
}
#endif

// all function operate on object pointer "_this"
blinkenlight_panel_list_t * blinkenlight_panels_constructor(void)
{
	blinkenlight_panel_list_t *_this;
	_this = (blinkenlight_panel_list_t *) malloc(sizeof(blinkenlight_panel_list_t));

	_this->panels_count = 0;
	return _this;

}

void blinkenlight_panels_destructor(blinkenlight_panel_list_t *_this)
{
	blinkenlight_panels_clear(_this) ;
	free(_this);
}

/*
 *	Clear all defined panels
 */
void blinkenlight_panels_clear(blinkenlight_panel_list_t *_this)
{
	int i_panel, i_control;
	// clear the whole static struct
	for (i_panel = 0; i_panel < MAX_BLINKENLIGHT_PANELS; i_panel++) {
		blinkenlight_panel_t *p = &(_this->panels[i_panel]);
		for (i_control = 0; i_control < MAX_BLINKENLIGHT_PANEL_CONTROLS; i_control++) {
			blinkenlight_control_t *c = &(p->controls[i_control]);
#ifdef BLINKENLIGHT_SERVER
			c->blinkenbus_register_wiring_count = 0;
#endif
		}
		p->controls_count = 0;
	}
	_this->panels_count = 0;
}

// new panel for global list
blinkenlight_panel_t *blinkenlight_add_panel(blinkenlight_panel_list_t *_this)
{
	int i;
	blinkenlight_panel_t *p;
	assert(_this->panels_count < MAX_BLINKENLIGHT_PANELS);
	i = _this->panels_count++; // handle is list index
	p = &(_this->panels[i]);
	p->index = i;
	p->controls_count = 0;
	p->controls_inputs_count = 0;
	p->controls_outputs_count = 0;
	p->controls_inputs_values_bytecount = 0;
	p->controls_outputs_values_bytecount = 0;
	p->default_radix = 10;
	p->name[0] = '\0';
	p->info[0] = '\0';
	p->controls_inputs_count = 0;
	p->mode = 0;
	p->tag = 0;
	return p;
}

// new control for a panel
blinkenlight_control_t *blinkenlight_add_control(blinkenlight_panel_list_t *_this,
		blinkenlight_panel_t *p)
{
	unsigned i;
	blinkenlight_control_t *c;
	assert(p->controls_count < MAX_BLINKENLIGHT_PANEL_CONTROLS);
	i = p->controls_count++; // handle is list index
	c = &(p->controls[i]);
	c->panel = p ;
	c->index = i;
	c->radix = p->default_radix;
	c->is_input = 1;
	c->type = 0;
	c->value = 0;
	c->value_previous = c->value;
	c->value_bitlen = 0;
	c->value_default = 0;
//@	c->mode = 0;
	c->tag = 0;

#ifdef BLINKENLIGHT_SERVER
	c->mirrored_bit_order = 0;
	c->blinkenbus_register_wiring_count = 0;
	c->fmax = 0 ;
	c->history = historybuffer_create(c, MAX_BLINKENLIGHT_HISTORY_ENTRIES) ;
	c->value_raw = 0 ;
#endif
	// is not free'd! but only single data struct allocate at program start
	return c;
}

#ifdef BLINKENLIGHT_SERVER
// new register_wiring for a control
blinkenlight_control_blinkenbus_register_wiring_t *blinkenlight_add_register_wiring(blinkenlight_control_t *c)
{
	int i;
	blinkenlight_control_blinkenbus_register_wiring_t *bbrw;
	assert(c->blinkenbus_register_wiring_count < MAX_BLINKENLIGHT_REGISTERS_PER_CONTROL);
	i = c->blinkenbus_register_wiring_count++;
	bbrw = &(c->blinkenbus_register_wiring[i]);
	bbrw->index = i;
	bbrw->blinkenbus_bitmask = 0 ;
	bbrw->blinkenbus_bitmask_len = 0 ;
	bbrw->blinkenbus_board_address = 0 ;
	bbrw->blinkenbus_levels_active_low = 0 ;
	bbrw->blinkenbus_lsb = 0;
    bbrw->blinkenbus_msb = 0;
    bbrw->board_register_address = 0;
    bbrw->board_register_space = 0;
	bbrw->blinkenbus_reversed = 0 ;
    bbrw->mux_code = 0;
	return bbrw;
}
#endif

/*
 *	search panel over its name. Case insensitive.
 */
blinkenlight_panel_t * blinkenlight_panels_get_panel_by_name(blinkenlight_panel_list_t *_this,
		char *panelname)
{
	unsigned i_panel;
	if (panelname == NULL || !strlen(panelname))
		return NULL ;
	for (i_panel = 0; i_panel < _this->panels_count; i_panel++) {
		blinkenlight_panel_t *p = &(_this->panels[i_panel]);
		if (!strcasecmp(panelname, p->name))
			return p; // found
	}
	return NULL ; // not found
}
/*
 * search a control over name (case insensitive), and input/output direction
 * Two phases:
 * 1) an exact name match is tried,
 * 2) an partial name match is tried, so "controlname" may be incomplete.
 * result is != nULL, if only one control has a name match
 */
blinkenlight_control_t * blinkenlight_panels_get_control_by_name(blinkenlight_panel_list_t *_this,
		blinkenlight_panel_t *p, char *controlname, int is_input)
{
	unsigned controlname_len;
	unsigned i_control;
	blinkenlight_control_t *c_candidate; // last control with partial name match
	unsigned candidate_count; // count of controls with partial name match
	if (p == NULL )
		return NULL ;
	if (controlname == NULL || !strlen(controlname))
		return NULL ;

	// 1) exact name match
	for (i_control = 0; i_control < p->controls_count; i_control++) {
		blinkenlight_control_t *c = &(p->controls[i_control]);
		if (c->is_input == is_input && !strcasecmp(controlname, c->name)) {
			// found one exact matching control name
			return c;
		}
	}

	// 2) partial name match
	controlname_len = strlen(controlname);
	c_candidate = NULL;
	candidate_count = 0;
	for (i_control = 0; i_control < p->controls_count; i_control++) {
		blinkenlight_control_t *c = &(p->controls[i_control]);
		if (c->is_input == is_input) {
			if (!strncasecmp(controlname, c->name, controlname_len)) {
				// found one partial matching control
				candidate_count++;
				c_candidate = c;
			}
		}
	}
	if (candidate_count == 1)
		return c_candidate;

	return NULL ; // not found
}
/*
 * count, how many controls have value !0 value->previous
 */
unsigned blinkenlight_panels_get_control_value_changes(blinkenlight_panel_list_t *_this,
		blinkenlight_panel_t *p, int is_input)
{
	unsigned i_control, n = 0;
	if (p == NULL )
		return 0;
	for (i_control = 0; i_control < p->controls_count; i_control++) {
		blinkenlight_control_t *c = &(p->controls[i_control]);
		if (c->is_input == is_input && c->value != c->value_previous)
			n++;
	}
	return n;
}

/*
 * get cha count  of longest control name
 */
unsigned blinkenlight_panels_get_max_control_name_len(blinkenlight_panel_list_t *_this,
		blinkenlight_panel_t *p)
{
	unsigned i_control, n = 0;
	if (p == NULL )
		return 0;
	for (i_control = 0; i_control < p->controls_count; i_control++) {
		blinkenlight_control_t *c = &(p->controls[i_control]);
		if (n < strlen(c->name))
			n = strlen(c->name);
	}
	return n;
}


#ifdef BLINKENLIGHT_SERVER
/*
 * Post processing after panels/controls have been defined
 * (either by read of config file, or by program code)
 */
void blinkenlight_panels_config_fixup(blinkenlight_panel_list_t *_this)
{
    unsigned i_panel, i_control, i_register_wiring;
    // eval lsb, msb bit ranges
    for (i_panel = 0; i_panel < _this->panels_count; i_panel++) {
        blinkenlight_panel_t *p = &(_this->panels[i_panel]);
        p->controls_inputs_count = 0;
        p->controls_outputs_count = 0;

        if (p->default_radix == 0)
            p->default_radix = 16; // default: hex representation

        for (i_control = 0; i_control < p->controls_count; i_control++) {
            uint64_t all_value_bits = 0;
            blinkenlight_control_t *c = &(p->controls[i_control]);
            // get radix from panel, if not defined
            if (c->radix == 0)
                c->radix = p->default_radix;
            // calc input/output direction
            if (BLINKENLIGHT_IS_OUTPUT_CONTROL(c->type)) {
                c->is_input = 0;
            } else
                c->is_input = 1;
            if (c->is_input)
                p->controls_inputs_count++;
            else
                p->controls_outputs_count++;

            if (c->blinkenbus_register_wiring_count > 0) {
                // no const value, set value_bitlen by register bit sum
                for (i_register_wiring = 0; i_register_wiring < c->blinkenbus_register_wiring_count;
                        i_register_wiring++) {
                    blinkenlight_control_blinkenbus_register_wiring_t *bbrw =
                            &(c->blinkenbus_register_wiring[i_register_wiring]);
                    // calc absolute blinkenbus address
                    bbrw->blinkenbus_register_address = (bbrw->blinkenbus_board_address << 4)
                            | bbrw->board_register_address;
                    // are lsb and msb reversed?
                    bbrw->blinkenbus_reversed = 0;
                    if (bbrw->blinkenbus_msb < bbrw->blinkenbus_lsb) {
                        unsigned char tmp = bbrw->blinkenbus_lsb;
                        bbrw->blinkenbus_lsb = bbrw->blinkenbus_msb;
                        bbrw->blinkenbus_msb = tmp;
                        bbrw->blinkenbus_reversed = 1;
                    }
                    // calc bitmap for range lsb .. msb
                    // mask the used bits from the blinkenbus register
                    bbrw->blinkenbus_bitmask_len =
                            (bbrw->blinkenbus_msb - bbrw->blinkenbus_lsb + 1);
                    bbrw->blinkenbus_bitmask = BitmaskFromLen32[bbrw->blinkenbus_bitmask_len]
                            << bbrw->blinkenbus_lsb;

                    ; // mask with bitmask_len bits from blinkenbus register

                    // mount all bit field from registers together into control
                    all_value_bits = mount_bits_to_mask64(all_value_bits, bbrw->blinkenbus_bitmask,
                            bbrw->blinkenbus_lsb, bbrw->control_value_bit_offset);
                }
                c->value_bitlen = get_msb_index64(all_value_bits) + 1;
            }
            // round bitlen up to bytes
            c->value_bytelen = (c->value_bitlen + 7) / 8; // 0-> 0, 1->1 8->1, 9->2, ...
        }
        // count total amount of bytes for all values of input/output controls
        // needed for compressed value transmission over RPC
        p->controls_inputs_values_bytecount = 0;
        p->controls_outputs_values_bytecount = 0;
        for (i_control = 0; i_control < p->controls_count; i_control++) {
            blinkenlight_control_t *c = &(p->controls[i_control]);
            if (c->is_input)
                p->controls_inputs_values_bytecount += c->value_bytelen;
            else
                p->controls_outputs_values_bytecount += c->value_bytelen;
        }
    }
}
#endif


/*
 * diagnostic dump
 */

void blinkenlight_panels_diagprint(blinkenlight_panel_list_t *_this, FILE *f)
{
	unsigned i_panel, i_control ;
#if defined(BLINKENLIGHT_SERVER)
	unsigned i_register;
#endif
// traverse the whole  struct
	for (i_panel = 0; i_panel < _this->panels_count; i_panel++) {
		blinkenlight_panel_t *p = &(_this->panels[i_panel]);
		fprintf(f, "Panel[%d]:\n", i_panel);
		fprintf(f, "  index = %d\n", p->index);
		fprintf(f, "  name   = \"%s\"\n", p->name);
		fprintf(f, "  info   = \"%s\"\n", p->info);
		for (i_control = 0; i_control < p->controls_count; i_control++) {
			blinkenlight_control_t *c = &(p->controls[i_control]);
			fprintf(f, "  control[%d]:\n", i_control);
			fprintf(f, "    index ....... = %u\n", c->index);
			fprintf(f, "    name ........ = \"%s\"\n", c->name);
			fprintf(f, "    type ........ = %s\n", blinkenlight_control_type_t_text(c->type));
			fprintf(f, "    radix ....... = %u\n", c->radix);
			fprintf(f, "    bit len ..... = %u\n", c->value_bitlen);
#if defined(BLINKENLIGHT_SERVER)
			fprintf(f, "    encoding .... = %s\n",
					blinkenlight_control_value_encoding_t_text( c->encoding));
            fprintf(f, "    bits mirrored = %u\n", c->mirrored_bit_order) ;
            fprintf(f, "    fmax ........ = %u\n", c->fmax) ;
			for (i_register = 0; i_register < c->blinkenbus_register_wiring_count; i_register++)
			{
				blinkenlight_control_blinkenbus_register_wiring_t *bbrw =
				&(c->blinkenbus_register_wiring[i_register]);
				fprintf(f, "    register_wiring[%d]:\n", i_register);
				fprintf(f, "      index ................. = %d\n", bbrw->index);
				fprintf(f, "      offset of 1st value bit = %d\n",
						bbrw-> control_value_bit_offset);
				fprintf(f, "      board address ......... = 0x%x\n", bbrw->blinkenbus_board_address);
				fprintf(f, "      register space ........ = %s\n",
						blinkenlight_register_space_t_text(bbrw->board_register_space));
				fprintf(f, "      register address ...... = 0x%x\n",
						bbrw->board_register_address);
				fprintf(f, "      register lsb..msb ..... = %d..%d\n",
						bbrw->blinkenbus_lsb, bbrw->blinkenbus_msb);
				fprintf(f, "      register pin polarity   = %s\n",
						bbrw->blinkenbus_levels_active_low ? "active LOW": "active HIGH");
				fprintf(f, "      register bit order .... = %s\n",
						bbrw->blinkenbus_reversed ? "reversed: 7..0" : "normal: 0..7");
				fprintf(f, "      register bitmap ....... = 0x%x (bit len = %d)\n",
						bbrw->blinkenbus_bitmask, bbrw->blinkenbus_bitmask_len);
			}
#endif
		}
	}

}

