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
#include "gpio.h"
#include "gpiopattern.h"

typedef unsigned int uint32;
typedef signed int int32;
typedef unsigned short uint16;

void short_wait(void); // used as pause between clocked GPIO changes
unsigned bcm_host_get_peripheral_address(void); // find Pi 2 or Pi's gpio base address
static unsigned get_dt_ranges(const char *filename, unsigned offset); // Pi 2 detect

extern int knobValue[2];	// value for knobs. 0=ADDR, 1=DATA. see main.c.
void check_rotary_encoders(int switchscan);

struct bcm2835_peripheral gpio; // needs initialisation

// long intervl = 300000; // light each row of leds this long
long intervl = 50000; // light each row of leds 50 us
// almost flickerfree at 32 phases

// static unsigned loopcount = 0 ;

// PART 1 - GPIO and RT process stuff ----------------------------------

// GPIO setup macros.
// In early versions INP_GPIO(x) was used always before OUT_GPIO(x),
// this is disabled now by INO_GPIO(g)
#define INO_GPIO(g) //INP_GPIO(g) // Use this before OUT_GPIO
#define INP_GPIO(g)   *(gpio.addr + ((g)/10)) &= ~(7<<(((g)%10)*3))
#define OUT_GPIO(g)   *(gpio.addr + ((g)/10)) |=  (1<<(((g)%10)*3))
#define SET_GPIO_ALT(g,a) *(gpio.addr + (((g)/10))) |= (((a)<=3?(a) + 4:(a)==4?3:2)<<(((g)%10)*3))

#define GPIO_SET  *(gpio.addr + 7)  // sets   bits which are 1 ignores bits which are 0
#define GPIO_CLR  *(gpio.addr + 10) // clears bits which are 1 ignores bits which are 0

#define GPIO_READ(g)  *(gpio.addr + 13) &= (1<<(g))

#define GPIO_PULL *(gpio.addr + 37) // pull up/pull down
#define GPIO_PULLCLK0 *(gpio.addr + 38) // pull up/pull down clock

// Pi 4 update
/* https://github.com/RPi-Distro/raspi-gpio/blob/master/raspi-gpio.c */	
/* 2711 has a different mechanism for pin pull-up/down/enable  */
#define GPPUPPDN0                57        /* Pin pull-up/down for pins 15:0  */
#define GPPUPPDN1                58        /* Pin pull-up/down for pins 31:16 */
#define GPPUPPDN2                59        /* Pin pull-up/down for pins 47:32 */
#define GPPUPPDN3                60        /* Pin pull-up/down for pins 57:48 */

// Exposes the physical address defined in the passed structure using mmap on /dev/mem
int map_peripheral(struct bcm2835_peripheral *p)
{
	if ((p->mem_fd = open("/dev/mem", O_RDWR | O_SYNC)) < 0) {
		printf("Failed to open /dev/mem, try checking permissions.\n");
		return -1;
	}
	p->map = mmap(
	NULL, BLOCK_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, p->mem_fd, // File descriptor to physical memory virtual file '/dev/mem'
			p->addr_p); // Address in physical map that we want this memory block to expose
	if (p->map == MAP_FAILED) {
		perror("mmap");
		return -1;
	}
	p->addr = (volatile unsigned int *) p->map;
	return 0;
}

void unmap_peripheral(struct bcm2835_peripheral *p)
{
	munmap(p->map, BLOCK_SIZE);
	close(p->mem_fd);
}

// PART 2 - the multiplexing logic driving the front panel -------------

uint8_t ledrows[] = {20, 21, 22, 23, 24, 25};
uint8_t rows[] = {16, 17, 18};
// pidp11 setup:
//uint8_t cols[] = {13, 12, 11,    10, 9, 8,    7, 6, 5,    4, 27, 26};
uint8_t cols[] = {26,27,4, 5,6,7, 8,9,10, 11,12,13};


void *blink(int *terminate)
{
	int i, j, k, switchscan, tmp;

	// Find gpio address (different for Pi 2) ----------
	gpio.addr_p = bcm_host_get_peripheral_address() + +0x200000;
	if (gpio.addr_p == 0x20200000)
		printf("*** RPi Plus detected\n");
	else if (gpio.addr_p == 0x3f000000)  
		printf("*** RPi 2/3/Z detected\n");
	else if (gpio.addr_p == 0xfe200000)  
		printf("*** RPi 4 detected\n");

	// printf("Priority max SCHED_FIFO = %u\n",sched_get_priority_max(SCHED_FIFO) );

	// set thread to real time priority -----------------
	struct sched_param sp;
	sp.sched_priority = 98; // maybe 99, 32, 31?
	if (pthread_setschedparam(pthread_self(), SCHED_FIFO, &sp)) {
		fprintf(stderr, "warning: failed to set RT priority\n");
	}

	// --------------------------------------------------
	if (map_peripheral(&gpio) == -1) {
		printf("Failed to map the physical GPIO registers into the virtual memory space.\n");
		return (void *) -1;
	}

	// initialise GPIO (all pins used as inputs, with pull-ups enabled on cols)

	for (i = 0; i < 6; i++) { // Define ledrows as input
		INP_GPIO(ledrows[i]);
		GPIO_CLR = 1 << ledrows[i]; // so go to Low when switched to output
	}
	for (i = 0; i < 12; i++) // Define cols as input
			{
		INP_GPIO(cols[i]);
	}
	for (i = 0; i < 3; i++) // Define rows as input
			{
		INP_GPIO(rows[i]);
	}

if (gpio.addr_p==0xfe200000)
{
	//printf("Configuring pullups for Pi 4\r\n");
	/* https://github.com/RPi-Distro/raspi-gpio/blob/master/raspi-gpio.c */	
	/* 2711 has a different mechanism for pin pull-up/down/enable  */
	int gpiox;
	int pullreg;
	int pullshift;
    unsigned int pullbits;
    unsigned int pull;

	// GPIO column pins
	for (i=0;i<12;i++)
	{
		gpiox = cols[i];
		pullreg = GPPUPPDN0 + (gpiox>>4);
		pullshift = (gpiox & 0xf) << 1;
		pull = 1;	// pullup

		pullbits = *(gpio.addr + pullreg);
		//printf("col %d pullreg %d pullshift %x pull %d -- pullbits %x --> ", gpiox, pullreg, pullshift, pull, pullbits);
		pullbits &= ~(3 << pullshift);
		pullbits |= (pull << pullshift);
		*(gpio.addr + pullreg) = pullbits;
		//printf("%x == %x --- %xl\r\n", pullbits, *(&gpio.addr_p + pullreg), gpio.addr_p + pullreg);
	}
	// GPIO row pins
	for (i=0;i<3;i++)
	{
		gpiox = rows[i];
		pullreg = GPPUPPDN0 + (gpiox>>4);
		pullshift = (gpiox & 0xf) << 1;
		pull = 0;	// pullup

		pullbits = *(gpio.addr + pullreg);
		pullbits &= ~(3 << pullshift);
		pullbits |= (pull << pullshift);
		*(gpio.addr + pullreg) = pullbits;
	}
	// GPIO ledrow pins
	for (i=0;i<6;i++)
	{
		gpiox = ledrows[i];
		pullreg = GPPUPPDN0 + (gpiox>>4);
		pullshift = (gpiox & 0xf) << 1;
		pull = 0;	// pullup

		pullbits = *(gpio.addr + pullreg);
		pullbits &= ~(3 << pullshift);
		pullbits |= (pull << pullshift);
		*(gpio.addr + pullreg) = pullbits;
	}
}
else 	// configure pullups for older Pis
{
	// BCM2835 ARM Peripherals PDF p 101 & elinux.org/RPi_Low-level_peripherals#Internal_Pull-Ups_.26_Pull-Downs
	GPIO_PULL = 2; // pull-up
	short_wait(); // must wait 150 cycles
	GPIO_PULLCLK0 = 0x0c003ff0; // selects GPIO pins 4..13 and 26,27

	short_wait();
	GPIO_PULL = 0; // reset GPPUD register
	short_wait();
	GPIO_PULLCLK0 = 0; // remove clock
	short_wait(); // probably unnecessary

	// BCM2835 ARM Peripherals PDF p 101 & elinux.org/RPi_Low-level_peripherals#Internal_Pull-Ups_.26_Pull-Downs
	GPIO_PULL = 0; // no pull-up no pull-down just float
	short_wait(); // must wait 150 cycles
	GPIO_PULLCLK0 = 0x03f00000; // selects GPIO pins 20..25
	short_wait();
	GPIO_PULL = 0; // reset GPPUD register
	short_wait();
	GPIO_PULLCLK0 = 0; // remove clock
	short_wait(); // probably unnecessary

	// BCM2835 ARM Peripherals PDF p 101 & elinux.org/RPi_Low-level_peripherals#Internal_Pull-Ups_.26_Pull-Downs
	GPIO_PULL = 0; // no pull-up no pull down just float
// not the reason for flashes it seems:
//GPIO_PULL = 2;	// pull-up - letf in but does not the reason for flashes
	short_wait(); // must wait 150 cycles
	GPIO_PULLCLK0 = 0x070000; // selects GPIO pins 16..18
	short_wait();
	GPIO_PULL = 0; // reset GPPUD register
	short_wait();
	GPIO_PULLCLK0 = 0; // remove clock
	short_wait(); // probably unnecessary
}
	// --------------------------------------------------

	// printf("\nPiDP-11 FP on\n");


	while (*terminate == 0) {
		unsigned phase;
//		if ((loopcount++ % 500) == 0)	printf("1\n"); // visual heart beat


		// display all phases circular
		for (phase = 0; phase < GPIOPATTERN_LED_BRIGHTNESS_PHASES; phase++) {
			// each phase must be eact same duration, so include switch scanning here

			// the original gpio_ledstatus[8] runs trough all phases
			volatile uint32_t *gpio_ledstatus =
					gpiopattern_ledstatus_phases[gpiopattern_ledstatus_phases_readidx][phase];

			// prepare for lighting LEDs by setting col pins to output
			for (i = 0; i < 12; i++) {
				INO_GPIO(cols[i]); //
				OUT_GPIO(cols[i]); // Define cols as output
			}

			// light up 6 rows of 12 LEDs each
			for (i = 0; i < 6; i++) {

				// Toggle columns for this ledrow (which LEDs should be on (CLR = on))
				for (k = 0; k < 12; k++) {
					if ((gpio_ledstatus[i] & (1 << k)) == 0)
						GPIO_SET = 1 << cols[k];
					else
						GPIO_CLR = 1 << cols[k];
				}

				// Toggle this ledrow on
				INO_GPIO(ledrows[i]);
				GPIO_SET = 1 << ledrows[i]; // test for flash problem
				OUT_GPIO(ledrows[i]);
				/*test* /			GPIO_SET = 1 << ledrows[i]; /**/

				nanosleep((struct timespec[]
				) {	{	0, intervl}}, NULL);

				// Toggle ledrow off
				GPIO_CLR = 1 << ledrows[i]; // superstition
//				INP_GPIO(ledrows[i]);
				usleep(10); // waste of cpu cycles but may help against udn2981 ghosting, not flashes though
			}

//nanosleep ((struct timespec[]){{0, intervl}}, NULL); // test

			// prepare for reading switches
			for (i = 0; i < 12; i++)
				INP_GPIO(cols[i]); // flip columns to input. Need internal pull-ups enabled.

			// read three rows of switches
			for (i = 0; i < 3; i++)
			{
				INO_GPIO(rows[i]); //			GPIO_CLR = 1 << rows[i];	// and output 0V to overrule built-in pull-up from column input pin
				OUT_GPIO(rows[i]); // turn on one switch row
				GPIO_CLR = 1 << rows[i]; // and output 0V to overrule built-in pull-up from column input pin

				nanosleep((struct timespec[]) { { 0, intervl / 100}}, NULL); // probably unnecessary long wait, maybe put above this loop also

				switchscan = 0;
				for (j = 0; j < 12; j++) // 12 switches in each row
				{
					tmp = GPIO_READ(cols[j]);
					if (tmp != 0)
						switchscan += 1 << j;
				}
				INP_GPIO(rows[i]); // stop sinking current from this row of switches

				if (i==2)
					check_rotary_encoders(switchscan);	// translate raw encoder data to switch position

				gpio_switchstatus[i] = switchscan;

			}
		}
	}

	//printf("\nFP off\n");
	// at this stage, all cols, rows, ledrows are set to input, so elegant way of closing down.

	return 0;
}

void short_wait(void) // creates pause required in between clocked GPIO settings changes
{
//	int i;
//	for (i=0; i<150; i++) {
//		asm volatile("nop");
//	}
	fflush(stdout); //
	usleep(1); // suggested as alternative for asm which c99 does not accept
}

unsigned bcm_host_get_peripheral_address(void) // find Pi's gpio base address
{
//	unsigned address = get_dt_ranges("/proc/device-tree/soc/ranges", 4);
//	return address == ~0 ? 0x20000000 : address;
// Pi 4 fix: https://github.com/raspberrypi/userland/blob/master/host_applications/linux/libs/bcm_host/bcm_host.c
   unsigned address = get_dt_ranges("/proc/device-tree/soc/ranges", 4);
   if (address == 0)
      address = get_dt_ranges("/proc/device-tree/soc/ranges", 8);
   return address == ~0 ? 0x20000000 : address;

}
static unsigned get_dt_ranges(const char *filename, unsigned offset)
{
	unsigned address = ~0;
	FILE *fp = fopen(filename, "rb");
	if (fp) {
		unsigned char buf[4];
		fseek(fp, offset, SEEK_SET);
		if (fread(buf, 1, sizeof buf, fp) == sizeof buf)
			address = buf[0] << 24 | buf[1] << 16 | buf[2] << 8 | buf[3] << 0;
		fclose(fp);
	}
	return address;
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
