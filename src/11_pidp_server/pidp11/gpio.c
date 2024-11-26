/* gpio.c: the real-time process that handles multiplexing

 Copyright (c) 2015-2016, Oscar Vermeulen & Joerg Hoppe
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

 18-Dec-2023  OV    new GPIO interface using the code of pinctrl, needed for Pi 5
 14-Aug-2019  OV    fix for Raspberry Pi 4's different pullup configuration
 01-Jul-2017  MH    remove INP_GPIO before OUT_GPIO and change knobValue
 01-Apr-2016  OV    almost perfect before VCF SE
 15-Mar-2016  JH    display patterns for brightness levels
 16-Nov-2015  JH    acquired from Oscar


 gpio.c from Oscar Vermeules PiDP8-sources.
 Slightest possible modification by Joerg.
 See www.obsolescenceguaranteed.blogspot.com

 The only communication with the main program (simh):
 external variable ledstatus is read to determine which leds to light.
 external variable switchstatus is updated with current switch settings.

 */

#define _GPIO_C_

#include <time.h>
#include <pthread.h>
#include <stdint.h>
//#include "gpio.h"
#include "gpiopattern.h"


//20231218
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include "pinctrl/gpiolib.h"
//-20231218


typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned short uint16;

void short_wait(void); // used as pause between clocked GPIO changes
extern int knobValue[2];	// value for knobs. 0=ADDR, 1=DATA. see main.c.
void check_rotary_encoders(int switchscan);

// long intervl = 300000; // light each row of leds this long
long intervl = 50000; // light each row of leds 50 us
// almost flickerfree at 32 phases

// PART 1 - GPIO and RT process stuff ----------------------------------


// PART 2 - the multiplexing logic driving the front panel -------------

uint8_t ledrows[] = {20, 21, 22, 23, 24, 25};
uint8_t rows[] = {16, 17, 18};
uint8_t cols[] = {26,27,4, 5,6,7, 8,9,10, 11,12,13};


void *blink(int *terminate)
{
	int i, j, k, switchscan, tmp;

	// init GPIO stuff ----------
        int num_gpios;
	int ret;

	ret = gpiolib_init();

	if (ret < 0)
	{
		printf("Failed to initialise gpiolib - %d\n", ret);
		return (void *)-1;
	}

	num_gpios = ret;
	if (!num_gpios)
	{
		printf("No GPIO chips found\n");
		return (void *)-1;
	}

	// -----
	uint32_t gpiomask[(MAX_GPIO_PINS + 31)/32] = { 0 };
#define ARRAY_SIZE(_a) (sizeof(_a)/sizeof(_a[0]))
	for (i = ARRAY_SIZE(gpiomask) - 1; i >= 0; i--)
	{
		if (gpiomask[i])
			break;
	}
	if (i < 0)
		memset(gpiomask, 0xff, sizeof(gpiomask));

	ret = gpiolib_mmap();
	if (ret)
	{
		if (ret == EACCES && geteuid())
			printf("Must be root\n");
		else
			printf("Failed to mmap gpiolib - %s\n", strerror(ret));
		return (void *)-1;
	}
	
	// set thread to real time priority -----------------
	struct sched_param sp;
	sp.sched_priority = 98; // maybe 99, 32, 31?
	if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp)) 
	{
		fprintf(stderr, "warning: failed to set RT priority\n");
	}

	// initialise GPIO (all pins used as inputs, with pull-ups enabled on cols)
/*	for (i = 0; i < 6; i++)  // Define ledrows as input
	for (i = 0; i < 12; i++) // Define cols as input
	for (i = 0; i < 3; i++) // Define rows as input
*/
	// GPIO column pins - input pullup
	// GPIO row pins - input pullup
	// GPIO ledrow pins - input pullup
	// --------------------------------------------------
	for (i=0;i<6;i++)
	{

		gpio_set_fsel(ledrows[i], GPIO_FSEL_OUTPUT);
		gpio_set_dir(ledrows[i], DIR_OUTPUT);
		gpio_set_drive(ledrows[i], DRIVE_LOW);
	}
	for (i=0;i<12;i++)
	{
		gpio_set_fsel(cols[i], GPIO_FSEL_OUTPUT);
		gpio_set_pull(cols[i], PULL_UP);
		gpio_set_dir(cols[i], DIR_OUTPUT);
		gpio_set_drive(cols[i], DRIVE_LOW);
	}
	for (i=0;i<3;i++)
	{
		gpio_set_fsel(rows[i], GPIO_FSEL_INPUT);
		//gpio_set_dir(rows[i], DIR_INPUT);
		gpio_set_pull(rows[i], PULL_UP);
	}

	printf("\nPiDP-11 FP 2023\n");

// ========== TEMPORARY TEST LOOP
	while (*terminate == 0) {
		unsigned phase;
		//unsigned loopcount=0;
		//if ((loopcount++ % 500) == 0)	printf("1\n"); // visual heart beat


		// display all phases circular
		for (phase = 0; phase < GPIOPATTERN_LED_BRIGHTNESS_PHASES; phase++) {
			// each phase must be eact same duration, so include switch scanning here

			// the original gpio_ledstatus[8] runs trough all phases
			volatile uint32_t *gpio_ledstatus =
					gpiopattern_ledstatus_phases[gpiopattern_ledstatus_phases_readidx][phase];

			// prepare for lighting LEDs by setting col pins to output
			for (i = 0; i < 12; i++) {
				gpio_set_dir(cols[i], DIR_OUTPUT);
			}

			// light up 6 rows of 12 LEDs each
			for (i = 0; i < 6; i++) {

				// Toggle columns for this ledrow (which LEDs should be on (CLR = on))
				for (k = 0; k < 12; k++) {
					if ((gpio_ledstatus[i] & (1 << k)) == 0)
						gpio_set_drive(cols[k], DRIVE_HIGH);
					else
						gpio_set_drive(cols[k], DRIVE_LOW);
				}

				// Toggle this ledrow on
				gpio_set_drive(ledrows[i], DRIVE_HIGH);

				nanosleep((struct timespec[]
				)	{	{	0, intervl}}, NULL);

				// Toggle ledrow off
				gpio_set_drive(ledrows[i], DRIVE_LOW);

				usleep(10); // waste of cpu cycles but may help against udn2981 ghosting, not flashes though
			}

			//nanosleep ((struct timespec[]){{0, intervl}}, NULL); // test

			// prepare for reading switches
			for (i = 0; i < 12; i++)			// flip columns to input 
				gpio_set_dir(cols[i], DIR_INPUT);	// Need intl pull-ups enabled

			// read three rows of switches
			for (i = 0; i < 3; i++)
			{
				// on one row pin, 
				// output 0V to overrule the built-in pull-up from column input pin
				// to read this row of switches
				gpio_set_dir(rows[i], DIR_OUTPUT);
				gpio_set_drive(rows[i], DRIVE_LOW);

				nanosleep((struct timespec[]) { { 0, intervl / 100}}, NULL); 
				// probably unnecessary long wait

				switchscan = 0;
				for (j = 0; j < 12; j++) // 12 switches in each row
				{
					tmp = gpio_get_level(cols[j]);
					if (tmp != 0)
						switchscan += 1 << j;
	
					//printf("-%d",tmp);
				}
				//printf(" | ");
				
				// stop sinking current from this row of switches
				gpio_set_dir(rows[i], DIR_INPUT);

				if (i==2)
					check_rotary_encoders(switchscan);	// translate raw encoder data to switch position

				gpio_switchstatus[i] = switchscan;

			}
			//printf("\n");
		}
	}
	//printf("\nFP off\n");
	// at this stage, all cols, rows, ledrows are set to input, so elegant way of closing down.

	return 0;
}

void short_wait(void) // creates pause required in between clocked GPIO settings changes
{
	fflush(stdout); //
	usleep(1); // suggested as alternative for asm which c99 does not accept
}



void check_rotary_encoders(int switchscan)
{
	// 2 rotary encoders. Each has two switch pins. Normally, both are 0 - no rotation.
	// encoder 1: row1, bits 8,9. Encoder 2: row1, bits 10,11
	// Gray encoding: rotate up sequence   = 11 -> 01 -> 00 -> 10 -> 11
	// Gray encoding: rotate down sequence = 11 -> 10 -> 00 -> 01 -> 11

	static int lastCode[2] = {3,3};
	int code[2];
	int i;

	code[0] = (switchscan & 0x300) >> 8;
	code[1] = (switchscan & 0xC00) >> 10;
	switchscan = switchscan & 0xff;	// set the 4 bits to zero

//printf("code 0 = %d, code 1 = %d\n", code[0], code[1]);

	// detect rotation
	for (i=0;i<2;i++)
	{
		if ((code[i]==1) && (lastCode[i]==3))
			lastCode[i]=code[i];
		else if ((code[i]==2) && (lastCode[i]==3))
			lastCode[i]=code[i];
	}

	// detect end of rotation
	for (i=0;i<2;i++)
	{
		if ((code[i]==3) && (lastCode[i]==1))
		{
			lastCode[i]=code[i];
			switchscan = switchscan + (1<<((i*2)+8));
//			printf("%d end of UP %d %d\n",i, switchscan, (1<<((i*2)+8)));
			knobValue[i]++;	//bugfix 20181225

		}
		else if ((code[i]==3) && (lastCode[i]==2))
		{
			lastCode[i]=code[i];
			switchscan = switchscan + (2<<((i*2)+8));
//			printf("%d end of DOWN %d %d\n",i,switchscan, (2<<((i*2)+8)));
			knobValue[i]--;	// bugfix 20181225
		}
	}


//	if (knobValue[0]>7)
//		knobValue[0] = 0;
//	if (knobValue[1]>3)
//		knobValue[1] = 0;
//	if (knobValue[0]<0)
//		knobValue[0] = 7;
//	if (knobValue[1]<0)
//		knobValue[1] = 3;

	knobValue[0] = knobValue[0] & 7;
	knobValue[1] = knobValue[1] & 3;

	// end result: bits 8,9 are 00 normally, 01 if UP, 10 if DOWN. Same for bits 10,11 (knob 2)
	// these bits are not used, actually. Status is communicated through global variable knobValue[i]

}
