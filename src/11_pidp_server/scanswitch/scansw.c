/*
 * Scan switches for PiDP-11 front panel
 * 
 * www.obsolescenceguaranteed.blogspot.com
 * 
 * v20231218
*/

#include <unistd.h>
#include <time.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
//#include "gpio.h"
#include "pinctrl/gpiolib.h"

void short_wait(void);								// used as pause between clocked GPIO changes
long intervl = 1000000;		// pause


// PART 1 - GPIO and RT process stuff ----------------------------------


 
// PART 2 - the multiplexing logic driving the front panel -------------

uint8_t row; 
uint8_t ledrows[] = {20, 21, 22, 23, 24, 25};
uint8_t rows[] = {16, 17, 18};
uint8_t cols[] = {26,27,4, 5,6,7, 8,9,10, 11,12,13};

int main()
{
	int i,j,k,switchscan[3], tmp;
	uint8_t exVal = 0;
	
	// init GPIO stuff ----------
        int num_gpios;
	int ret;

	ret = gpiolib_init();

	if (ret < 0)
	{
		printf("Failed to initialise gpiolib - %d\n", ret);
		return -1;
	}

	num_gpios = ret;
	if (!num_gpios)
	{
		printf("No GPIO chips found\n");
		return -1;
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
		return -1;
	}

	// initialise GPIO (all pins used as inputs, with pull-ups enabled on cols)
	for (i=0;i<6;i++)
	{

		gpio_set_fsel(ledrows[i], GPIO_FSEL_INPUT);
		//gpio_set_dir(ledrows[i], DIR_OUTPUT);
		//gpio_set_drive(ledrows[i], DRIVE_LOW);
	}
	for (i=0;i<12;i++)
	{
		gpio_set_fsel(cols[i], GPIO_FSEL_INPUT);
		gpio_set_pull(cols[i], PULL_UP);
		//gpio_set_dir(cols[i], DIR_OUTPUT);
		//gpio_set_drive(cols[i], DRIVE_LOW);
	}
	for (i=0;i<3;i++)
	{
		gpio_set_fsel(rows[i], GPIO_FSEL_INPUT);
		//gpio_set_dir(rows[i], DIR_INPUT);
		gpio_set_pull(rows[i], PULL_UP);
	}

	// --------------------------------------------------


	// prepare for reading switches		

	for(row = 0; row < 2; row++) {
		// on one row pin, 
		// output 0V to overrule the built-in pull-up from column input pin
		// to read this row of switches
		gpio_set_dir(rows[row], DIR_OUTPUT);
		gpio_set_drive(rows[row], DRIVE_LOW);

		nanosleep ((struct timespec[]){{0, intervl/100}}, NULL); // probably unnecessary long wait
		switchscan[row]=0;

		for(j=0;j<12;j++) {		// 12 switches in each row
			tmp = gpio_get_level(cols[j]);

			if(tmp==0)
				switchscan[row] += 1<<j;// only allows for one switch (last one) to be detected in each row
		}
		gpio_set_dir(rows[row], DIR_INPUT);

	}

	printf("%d\n", switchscan[1]<<12 | switchscan[0]);
	return switchscan[0];
}


void short_wait(void)				// creates pause required in between clocked GPIO settings changes
{
	fflush(stdout); //
	usleep(100000); // suggested as alternative for asm which c99 does not accept
}
