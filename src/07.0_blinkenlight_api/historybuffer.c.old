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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <inttypes.h>

#ifdef WIN32
#include <windows.h>
#else
#include <time.h>
#endif
#include "historybuffer.h"
#include "blinkenlight_panels.h"

#define ISEMPTY(this)  ((this)->readidx == (this)->writeidx)

// need it here
static uint64_t maxu64(uint64_t a, uint64_t b)
{
    if (a > b)
        return a;
    else
        return b;
}

/*
 * high resolution system ticks in micro seconds
 */
uint64_t historybuffer_now_us()
{
#ifdef WIN32
    // QueryPerformanceFrequency() and QueryPerformanceCounter() are
    return 0;
#else
    // Linuxes
    struct timespec res, tp;
// Linux hires timestamp: clock_getres() and clock_gettime()
// not available on all systems ?
    assert(!clock_getres(CLOCK_MONOTONIC_RAW, &res));
    assert(res.tv_sec == 0);
    assert(res.tv_nsec < 100000); // resolution must be better than 0.1 microsec
    assert(!clock_gettime(CLOCK_MONOTONIC_RAW, &tp));
    return (uint64_t) tp.tv_sec * 1000000 + (uint64_t) tp.tv_nsec / 1000;

#endif
}

// print a buffer entry
static char *historybuffer_entry_as_text(historybuffer_entry_t *hbe)
{
    static char buffer[256];
    if (hbe->timestamp_end_us == 0)
        sprintf(buffer, "timestamp=%0.3f us (unterminated), value=0%" PRIo64,
                (double) hbe->timestamp_begin_us / 1000, hbe->value);
    else
        sprintf(buffer, "timestamp=%0.3f - %0.3f = %0.3f us, value=0%" PRIo64,
                (double) hbe->timestamp_begin_us / 1000, (double) hbe->timestamp_end_us / 1000,
                (double) (hbe->timestamp_end_us - hbe->timestamp_begin_us) / 1000, hbe->value);
    return buffer;
}

/*
 * create(), destroy()
 * buffer is for one value, bitlen is const.
 * 0: is no bit vector, see historybuffer_get_average_vals()
 */
historybuffer_t *historybuffer_create(struct blinkenlight_control_struct *c, unsigned capacity)
{
    historybuffer_t *_this;
    _this = (historybuffer_t *) malloc(sizeof(historybuffer_t));
    _this->control = c;
    _this->capacity = (int) capacity;
    assert(_this->capacity > 0);
    _this->buffer = (historybuffer_entry_t *) calloc(capacity, sizeof(historybuffer_entry_t));
    _this->startpos = _this->endpos = 0;
    return _this;
}

void historybuffer_destroy(historybuffer_t *_this)
{
    free(_this->buffer);
    free(_this);
}

// get number of items in buffer
unsigned historybuffer_fill(historybuffer_t *_this)
{
    if (_this->startpos == _this->endpos)
        return 0;
    else if (_this->endpos > _this->startpos)
        return _this->endpos - _this->startpos;
    else
        // end rolled around
        return _this->capacity - _this->startpos + _this->endpos;
}

// internal: translate an item index into buffer position.
// idx < fill() !
static unsigned historybuffer_idx2pos(historybuffer_t *_this, unsigned idx)
{
    int pos;
    assert(_this->startpos != _this->endpos); // not empty
    if (_this->endpos > _this->startpos) {
        assert(idx < _this->endpos);
        pos = _this->startpos + idx;
    } else { // end rolled around
        if (idx < _this->capacity - _this->startpos)
            // idx not rolled around
            pos = _this->startpos + idx;
        else {
            // start at ring end, end and idx rolled around
            pos = idx - (_this->capacity - _this->startpos); // index now from buffer[0]
            assert(pos < _this->endpos); // not behind end ptr
        }
    }
    return pos;
}

// get item over linear index. [0] = oldest, [fill()-1] = newest, NULL if not found
historybuffer_entry_t *historybuffer_get(historybuffer_t *_this, unsigned idx)
{
    if (idx >= historybuffer_fill(_this))
        return NULL;
    else {
        unsigned pos = historybuffer_idx2pos(_this, idx);
        return &(_this->buffer[pos]);
    }
}

/* get oldest entry.
 */
historybuffer_entry_t * historybuffer_peek_first(historybuffer_t *_this)
{
    if (_this->startpos == _this->endpos)
        return NULL; // empty ;
    return &_this->buffer[_this->startpos];
}

historybuffer_entry_t * historybuffer_peek_last(historybuffer_t *_this)
{
    int pos;
    if (_this->startpos == _this->endpos)
        return NULL; // empty ;
    pos = _this->endpos - 1; // point to last written
    if (pos < 0)
        pos = _this->capacity - 1;
    return &_this->buffer[pos];
}

// remove oldest item from start of buffer
historybuffer_entry_t * historybuffer_poll(historybuffer_t *_this)
{
    historybuffer_entry_t *entry;
    if (_this->startpos == _this->endpos)
        return NULL; // empty ;
    entry = &_this->buffer[_this->startpos];
    _this->startpos++;
    if (_this->startpos >= _this->capacity)
        _this->startpos = 0;
    return entry; // valid until next push()
}

/* append new value at end of buffer
 * oldest entries get pushed out
 * the end timestamp of the current value (last set) is udpated ti now
 * See Java code at blinkenbone.panelsim.ControlSliceVisualization.setStateAveraging()
 */
void historybuffer_set_val(historybuffer_t *_this, uint64_t now_us, uint64_t value)
{
    historybuffer_entry_t *hbe;

    if (historybuffer_fill(_this) + 1 >= _this->capacity)
        // overflow: remove oldest
        historybuffer_poll(_this);

    hbe = historybuffer_peek_last(_this);
    if (hbe == NULL || hbe->value != value) {
        // add new entry only if state changed!
        if (hbe != NULL)
            // terminate current state, if queue not empty
            hbe->timestamp_end_us = now_us;
        // append new state
        hbe = &_this->buffer[_this->endpos];
        // advance to next free buffer entry
        _this->endpos++;
        if (_this->endpos >= _this->capacity) {
            _this->endpos = 0;
        }
        // fill data
        hbe->timestamp_begin_us = now_us;
        hbe->timestamp_end_us = 0; // currently valid.
        hbe->value = value;
    }
}

/*
 * calculate average level for every value bit
 * - averaging_interval_us: average values for this time interval are calculated
 * - now_us: current time provided by caller, generated with historybuffer_now_us()
 * - bitmode: 1: averaging is done for each bit in hisotry separately
 * 			then result is returned in 'control->averaged_value_bits[]'
 * 		if 0: averaging is donw for the whole value
 * 			then result is returned in 'control->averaged_value'
 *
 * averaged_value_bits is an array of uint8_t, with bit indexes.
 * Fractional scale: 1.0 <=> 255, 0 <=> 0
 * Two modes: "value mode" and "bit mode"
 * "Bit mode" average each bit separately.
 * So if the input pattern in the average time interval is 0x00, 0x01, 0x02, 0x03, 0x05 ...
 *    bit 0: averaged_value_bits[0] := 153 <=> 3/5
 *    bit 1: averaged_value_bits[1] := 102 <=> 2/5
 *    bit 2: averaged_value_bits[2] :=  51 <=> 1/5
 * If _this_valuebitlen == 0:
 * averaged_value := 561 <=> (0+1+2+3+5)/5 = 11/5
 *
 * if averaging_interval_us == 0: return current value
 *
 * See Java code at blinkenbone.panelsim.ControlSliceVisualization.getState()
 */
void historybuffer_get_average_vals(historybuffer_t *_this, uint64_t averaging_interval_us,
        uint64_t now_us, int bitmode)
{
    int last_idx; // index of newest entry in history
    unsigned fill;
    unsigned idx;
    historybuffer_entry_t *hbe;
    uint64_t interval_start_us;
    uint64_t sum_state_durations[64]; // cummulated time of all ON states
    uint64_t sum_durations_us; // time of all states
    unsigned bitidx;

    // defaults: all 0
    memset(_this->control->averaged_value_bits, 0, sizeof(_this->control->averaged_value_bits));
    _this->control->averaged_value = 0;

    last_idx = historybuffer_fill(_this) - 1;
    if (last_idx < 0)
        return; // buffer empty, return all 0's

    hbe = historybuffer_get(_this, last_idx);
    assert(hbe != NULL);

    if (averaging_interval_us == 0) {
        // just return the current value
        _this->control->averaged_value = hbe->value;
        if (bitmode)
            for (bitidx = 0; bitidx < _this->control->value_bitlen; bitidx++)
                if ((hbe->value >> bitidx) & 1)
                    _this->control->averaged_value_bits[bitidx] = 1;
        return;
    }

    // terminate current state (which has no end time yet)
    hbe->timestamp_end_us = now_us;

    // discard old history entries,
    interval_start_us = now_us - averaging_interval_us;
    while ((hbe = historybuffer_peek_first(_this)) && hbe->timestamp_end_us <= interval_start_us)
        historybuffer_poll(_this);
    // update start time of first entry to start of averaging intervall (truncate partly)
    // not necessary, see use of "max()" below. But better for debugging output.
    if (hbe)
        hbe->timestamp_begin_us = interval_start_us;

    memset(sum_state_durations, 0, sizeof(sum_state_durations));
    sum_durations_us = 0;
    fill = historybuffer_fill(_this);
    // iterate through all entries in time interval
    for (idx = 0; (hbe = historybuffer_get(_this, idx)) != NULL; idx++) {
        uint64_t state_starttime_us = maxu64(interval_start_us, hbe->timestamp_begin_us);
        uint64_t state_duration_us = hbe->timestamp_end_us - state_starttime_us;
        if (!bitmode) {
            // average whole value. Overflow calculation of 64 bit arithmetic:
            // assume 256 buffer entries and a final scale of 255, so 8+8 extra bits are needed.
            // => overflow if value > 2^48 => 36 bit PDP-10 OK
            sum_state_durations[0] += hbe->value * state_duration_us;
        } else
            // average single bits
            for (bitidx = 0; bitidx < _this->control->value_bitlen; bitidx++) {
                if ((hbe->value >> bitidx) & 1)
                    sum_state_durations[bitidx] += state_duration_us;
            }
        sum_durations_us += state_duration_us;
    }

    // calc average into result
    // !! sumDurations_us == (now_us - intervalStart_us !!
    if (_this->control->value_bitlen == 0) {
        // average whole value
        _this->control->averaged_value = (255 * sum_state_durations[0]) / sum_durations_us;
    } else
        // average every single bit
        for (bitidx = 0; bitidx < _this->control->value_bitlen; bitidx++) {
            if (sum_durations_us > 0)
                // falls in range 0..255
                _this->control->averaged_value_bits[bitidx] = (255 * sum_state_durations[bitidx])
                        / sum_durations_us;
            else
                _this->control->averaged_value_bits[bitidx] = 0;
        }
}

/*
 * dump buffer.
 * if 'test_average' : calc average for sime timevalue in the middle of the buffer timestamps.
 * ! For tests, set capacity very small (16)
 */
void historybuffer_diagdump(historybuffer_t *_this, FILE *stream, int test_average)
{
    unsigned idx;
    historybuffer_entry_t *hbe;
    unsigned fill;

    fprintf(stream, "Dump of historybuffer @ %p\n", _this);
    fill = historybuffer_fill(_this);
    fprintf(stream, "  capacity=%u, startpos=%u, endpos=%u, fill=%u\n", _this->capacity,
            _this->startpos, _this->endpos, fill);
    fprintf(stream, "  first: capacity=%u, startpos=%u, endpos=%u, fill=%u\n", _this->capacity,
            _this->startpos, _this->endpos, fill);

    hbe = historybuffer_peek_first(_this);
    if (hbe == NULL)
        fprintf(stream, "no first entry\n");
    else
        fprintf(stream, "first entry: %s\n", historybuffer_entry_as_text(hbe));

    hbe = historybuffer_peek_last(_this);
    if (hbe == NULL)
        fprintf(stream, "no last entry\n");
    else
        fprintf(stream, "last entry: %s\n", historybuffer_entry_as_text(hbe));

    for (idx = 0; idx < fill; idx++) {
        unsigned pos = historybuffer_idx2pos(_this, idx);
        hbe = &(_this->buffer[pos]);
        fprintf(stream, "entry[%u]: pos=%u, %s\n", idx, pos, historybuffer_entry_as_text(hbe));
    }

    if (test_average && fill > (_this->capacity * 2 / 3)) {
        uint64_t now_us = historybuffer_now_us();
        uint64_t intervall_start_us;
        uint64_t averaging_interval_us;
        int bitidx; // decrements
        // calc average for middle of sampled timestamps. after that, older buffer entries are deleted
        idx = fill / 2;
        hbe = historybuffer_get(_this, idx);
        if (hbe == NULL)
            return;
        // interval start in the middle of a value sample
        intervall_start_us = (hbe->timestamp_end_us + hbe->timestamp_begin_us) / 2;
        averaging_interval_us = now_us - intervall_start_us;
        historybuffer_get_average_vals(_this, averaging_interval_us, now_us, /*bitmode*/1);
        fprintf(stream,
                "Average vals from timestamp = %0.3f to now = %0.3f = delta %0.3f us. avg[%u..0]:\n",
                (double) hbe->timestamp_begin_us / 1000, (double) now_us / 1000,
                (double) averaging_interval_us / 1000, _this->control->value_bitlen - 1);
        for (bitidx = _this->control->value_bitlen; bitidx >= 0; bitidx--) {
            fprintf(stream, "[%u]=%0.2f ", bitidx,
                    (double) (_this->control->averaged_value_bits[bitidx]) / 255);
        }
        fprintf(stream, "\n");
    }

}

