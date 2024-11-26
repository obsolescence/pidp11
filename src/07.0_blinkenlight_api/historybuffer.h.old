/* historybuffer.h: FIFO for blinkenlight APi control value history

 Copyright (c) 2016, Joerg Hoppe
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


 12-Mar-2016	JH      created
 */

#ifndef HISTORYBUFFER_H_
#define HISTORYBUFFER_H_

#include <stdint.h>
#include <stdio.h>

/* single entry in history buffer */
typedef struct
{
	uint64_t value;
	uint64_t timestamp_begin_us; // value is valid after this time
	uint64_t timestamp_end_us; // value is valid before this time, 0 = current valid
} historybuffer_entry_t;

typedef struct
{
	struct blinkenlight_control_struct *control; // backlink to possessing control
	// data types internal "int" becaus of arithemtic
	int capacity;

	int startpos; // first valid item in ring
	int endpos; // next free item to write
	// empty: readidx == writeidx
	historybuffer_entry_t *buffer;
} historybuffer_t;

uint64_t historybuffer_now_us(void);

historybuffer_t *historybuffer_create(struct blinkenlight_control_struct *c, unsigned capacity);
void historybuffer_destroy(historybuffer_t *_this);

// append new value at end of buffer
void historybuffer_set_val(historybuffer_t *_this, uint64_t now_us, uint64_t value);

// get oldest entry
historybuffer_entry_t * historybuffer_peek_first(historybuffer_t *_this);
// get newest entry
historybuffer_entry_t * historybuffer_peek_last(historybuffer_t *_this);

// remove oldest item from start of buffer
historybuffer_entry_t * historybuffer_poll(historybuffer_t *_this);

// get number # of items in buffer
unsigned historybuffer_fill(historybuffer_t *_this);

// get item over index. 0 = oldest, fill()-1 = newest, NULL if not found
historybuffer_entry_t *historybuffer_get(historybuffer_t *_this, unsigned idx);

void historybuffer_get_average_vals(historybuffer_t *_this, uint64_t averaging_interval_us,
		uint64_t now_us, int bitmode);

void historybuffer_diagdump(historybuffer_t *_this, FILE *stream, int test_average);

#endif /* HISTORYBUFFER_H_ */
