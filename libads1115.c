/* Preface
 * libads1115 - library for the Texas Instrument ADS1115 analog-digital
 * converter chip.  Technically, the library should work with the TI
 * ADS1x15/ADS111x chips as well, however, in cases where the chips
 * behaved differently, the code assumes a 1115 and defaults to whatever
 * the 1115 would expect.

 * Dev Environment:
 * 		Raspbian GNU/Linux 8 (jessie)
 * 		gcc (Raspbian) 4.9.2
 * 		Geany 1.24.1
 *
 * Library makes use of Raspberry Pi hardware, therefore is not portable
 * to other systems as-is. Notably, ports of this code will have to
 * deal with communications with the ADS1115 via I2C in the new system.
 *
 * Chris L Wilson - 17.05.2017
 * 
 * Modified by: Charles Degrapharee Exum
 * Date: 2018.10.13
 * Contact: charlesexumdev@gmail.com
 * 
 */

#include <stdio.h>			// perror(), printf() family, ect.
#include <inttypes.h>		// uint8_t, ect.

// i2c required headers
#include <unistd.h>			// read(), write(), usleep()
#include <fcntl.h>			// filecontrol - open()
#include <linux/i2c-dev.h> 	// I2C bus definitions
#include <sys/ioctl.h>		// ioctl()

#include "libads1115.h"		// header for this file

uint8_t 	outBuf[3] = {0};// holds bytes to send to device
uint8_t 	inBuf[2] = {0}; // holds bytes received from device

// module level variables needed between functions
float 		adcRes = 0.0;	// A/D resolution
int			devADC = 0;		// device file descriptor

// bit masks for various adc settings
uint8_t 	chanMask = 0b01000000;	// default to channel 0
uint8_t 	rateMask = 0b00000000;	// default to 8 sps
uint8_t 	multMask = 0b00000010;	// default to +/- 4.096V range

void close_1115() {
	if(close(devADC)) { // closes the file opened by init
		perror("close_1115");
	}
} // close_1115

int init_1115(int devID, int address) {
// returns file descriptor used to communicate with device
// requires address and device filename ID
// devID is part of the filename assigned by Linux OS i.e. /dev/i2c-<devID>)

	// clip to valid ADS1115 address values
	if (address < 0x48) address = 0x48;
	if (address > 0x4B) address = 0x4B;

	// force devID to 0-255
	if(devID < 0) devID = 0;
	if(devID > 255) devID = 255;

	char devName[15];	// linux file name for the device
	sprintf(devName, "/dev/i2c-%i", devID);
	// open i2c data stream
	devADC = open(devName, O_RDWR);		// opens device for r/w
	if (devADC < 0) {	// open() returns -1 on error, non-negative int file descriptor on success
		perror("init_1115 - open");
	}
	
	// connect to ads1115 as i2c slave
	if (ioctl(devADC, I2C_SLAVE, address) < 0) {	// setup device specific settings
		perror("init_1115 - ioctl I2C_SLAVE connect");
	}
	// console output for debugging
	printf("ADS1115 - Successfully opened %s @ 0x%02X as I2C_SLAVE\n", devName, address);

	return(devADC);
} // init

// Set ADS1115 active channel
void setCH_1115(int channel) {
	// only supports single ended for now
	// modifies bits in config reg that pertain to Channel mux
	switch (channel) {
		default:	channel=0;	//anything outside the range 0-3 gets treated as 0
		case 0 : 	chanMask = 0b01000000;
					break;
		case 1 : 	chanMask = 0b01010000;
					break;
		case 2 : 	chanMask = 0b01100000;
					break;
		case 3 : 	chanMask = 0b01110000;
					break;
	} // switch
	printf("ADS1115 - Measuring channel %d\n", channel); // console for debugging
} // setChannel

// Set ADS1115 data collection rate
int setSPS_1115(enum convRate_1115 rate) {
	// modifies bits in config reg that pertain to data rate
	switch (rate) {
		case SPS8:
			rateMask = 0b00000000;
			break;
		case SPS16:
			rateMask = 0b00100000;
			break;
		case SPS32:
			rateMask = 0b01000000;
			break;
		case SPS64:
			rateMask = 0b01100000;
			break;
		case SPS128:
			rateMask = 0b10000000;
			break;
		case SPS250:
			rateMask = 0b10100000;
			break;
		case SPS475:
			rateMask = 0b11000000;
			break;
		case SPS860:
			rateMask = 0b11100000;
			break;
	} // switch
	printf("ADS1115 - %d SPS\n", (int)rate); // console for debugging
	return((int)rate);
} // setSPS

// Set ADS1115 programmable gain amplifier
void setPGA_1115(enum configPGA_1115 pga) {
	float range;
	switch (pga) {
		case PGA6_144:
			multMask = 0b00000000;
			range = 6.144;		//**NOTE actual range is limited to +/- Vcc+0.3 volts (Vcc normally 3.3V)
			break;
		case PGA4_096:
			multMask = 0b00000010;
			range = 4.096;		//**NOTE actual range is limited to +/- Vcc+0.3 volts (Vcc normally 3.3V)
			break;
		case PGA2_048:
			multMask = 0b00000100;
			range = 2.048;
			break;
		case PGA1_024:
			multMask = 0b00000110;
			range = 1.024;
			break;
		case PGA0_512:
			multMask = 0b00001000;
			range = 0.512;
			break;
		case PGA0_256:
			multMask = 0b00001010;
			range = 0.256;
			break;
	} // switch
	adcRes = range / 32768.0;
	printf("ADS1115 - +/-%1.4fV with %0.5fmV resolution\n", range, adcRes*1000.0); // console for debugging
} // setPGA

// triggers a conversion and reads in a voltage
double readVolts_1115(int devHandle) {
	//*** setup ADC for single conversion - see config register notes in libads1115.h
	// setup output buffer defaults
	outBuf[0] = 0b00000001;	//pointer reg - set to config
	outBuf[1] = 0b10000001;	// bits 15-8 config register
	outBuf[2] = 0b00000011; // bits 7-0 config register

	// apply bitmasks for channel, input multiplier (PGA), and data rate
	//outBuf[1] &= 0b10000001;			// clear the bits used for channel and pga
	outBuf[1] |= chanMask | multMask;	// turn on bits for desired channel and pga
	//outBuf[2] &= 0b00011111;			// clear the bits used for rate
	outBuf[2] |= rateMask;				// turn on bits for desired rate

	// write config values & trigger acq (setting bit 7 in outBuf[1] triggers acq)
	if (write(devHandle, outBuf, 3) != 3) {
		perror("Write A/D config values.");
		return(-10.0);
	}

	//wait for conversion complete - revisit, does not protect from infinite loop here
	do {
		if (read(devHandle, inBuf, 2) != 2) {	// reads two bytes in from device into inBuf[]
			  perror("Read conversion done bit");
			  return(-10.0);
		}
	} while ((inBuf[0] & 0b10000000) == 0);			// waits for done bit (15) to go high

	// *** read conversion register
	outBuf[0] = 0;   // point to conversion reg
	if (write(devHandle, outBuf, 1) != 1) { // write out config to enable reading the data
		perror("Setup to read conversion");
		return(-10.0);
	}
	if (read(devHandle, inBuf, 2) != 2) { // read in the 16 bits acquired as a 2 byte array
		perror("Read conversion");
		return(-10.0);
	}
	// combine the two bytes into single unsigned 16 bit value
	uint16_t retcode = (inBuf[0] << 8 | inBuf[1]); 
	int sign = 1;
	if(retcode >= 32768) {	//two's complement conversion
		retcode = (~retcode + 1);
		sign = -1;
	}	
	// convert to voltage by multiplying by A/D resolution and applying sign
	double volts = (retcode * adcRes * sign);
	printf("ADS1115 - measure %1.7f\n", volts);	// console for debugging
	return(volts);
} // readVolts

// averages multiple conversions
double avgReads(int numReads) {
	double avg = 0.0;
	for(int x = 0; x < numReads; x++) {
		avg += readVolts_1115(devADC); // sum up individual readings
	} // for x = 1 to numreads
	avg /= numReads; // take the average
	printf("ADS1115 - avg = %1.7f (%d readings)\n", avg, numReads);
	return(avg);
} // avgReads
