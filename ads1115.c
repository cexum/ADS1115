/* Developer:	Charles DeGrapharee Exum
 * Date:		2018.10.12
 * Contact:		charlesexumdev@gmail.com
 * Purpose: 	library for Texas Instruments ADS1115 adc chip
 * 
 * Preface:		add1115.c - library for the Texas Instrument ADS1115
 * 				analog-digital converter chip.  Technically, the library
 * 				should work with the TI ADS1x15/ADS111x chips as well,
 * 				however, in cases where the chips behaved differently,
 * 				the code assumes a 1115 and defaults to whatever the
 * 				1115 would expect. Library makes use of Raspberry Pi
 * 				hardware, therefore is not portable to other systems
 * 				as-is. Notably, ports of this code will have to deal
 * 				with communications with the ADS1115 via I2C in the new
 * 				system.
 * 
 * 				Currently not implementing continuous conversion mode.
 * 
 * Written on:	Raspbian GNU/Linux 8 (jessie)
 * 				gcc (Raspbian) 4.9.2
 * */
#include "ads1115.h"

// pointer mask 
uint8_t ui8_pointer_register_mask = POINTER_REGISTER_CONFIG; 

// config masks
uint8_t ui8_config_register_operation_mask = CONFIG_REGISTER_IDLE; 
uint8_t	ui8_config_register_mult_mask = CONFIG_REGISTER_MULT_AIN_0; 
uint8_t ui8_config_register_pga_mask = CONFIG_REGISTER_PGA_4_096;
uint8_t ui8_config_register_conversion_mode_mask = CONFIG_REGISTER_SINGLE_CONVERSION;
uint8_t ui8_config_register_conversion_rate_mask = CONFIG_REGISTER_SPS_32;
uint8_t ui8_config_register_comparator_mode_mask = CONFIG_REGISTER_COMPARATOR_MODE_TRADITIONAL;
uint8_t ui8_config_register_comparator_polarity_mask = CONFIG_REGISTER_COMPARATOR_POLARITY_LOW;
uint8_t ui8_config_register_comparator_latch_mask = CONFIG_REGISTER_COMPARATOR_NON_LATCHING;
uint8_t ui8_config_register_comparator_queue_mask = CONFIG_REGISTER_COMPARATOR_QUEUE_DISABLED;

uint8_t ui8arr_write_buffer[3];
uint8_t ui8arr_read_buffer[2];

int i_ADS1115_handle;

float f_resolution = (PGA_4_096V / 32768.0);
	
void ADS1115_close() {
	puts("ADS1115_close");	
	if (close(i_ADS1115_handle)) {
		perror("ADS1115_close");
		}	
	}
	
void ADS1115_set_pointer_register(enum POINTER_MASK mode) {
	puts("ADS1115_set_pointer_register");
	ui8_pointer_register_mask = mode;
	printf("pointer register mask set to: %d\n", mode);
	}
	
void ADS1115_set_conversion_rate(enum RATE_MASK sps) {
	puts("ADS1115_set_conversion_rate");	
	ui8_config_register_conversion_rate_mask = sps;	
	printf("conversion rate mask set to: %d\n", ui8_config_register_conversion_rate_mask);
	}
	
void ADS1115_set_conversion_mode(enum CONVERSION_MODE_MASK mode) {
	puts("ADS1115_set_conversion_mode");
	ui8_config_register_conversion_mode_mask = mode;
	printf("conversion rate mask set to: %d\n", ui8_config_register_conversion_mode_mask);
	puts("warning: CONVERSION_MODE_MASK is ignored -- currently single shot only!");
	}
	
void ADS1115_set_multiplex(enum MULT_MASK mult) {
	puts("ADS1115_set_multiplex");
	ui8_config_register_mult_mask = mult;
	printf("multiplex mask set to: %d\n", ui8_config_register_mult_mask);
	}
	
void ADS1115_set_comparator_mode(enum COMPARATOR_MODE_MASK mode) {
	puts("ADS1115_set_comparator_mode");
	ui8_config_register_comparator_mode_mask = mode;
	printf("comparator mode mask set to: %d\n", ui8_config_register_comparator_mode_mask);
	}
	
void ADS1115_set_comparator_polarity(enum COMPARATOR_POLARITY_MASK polarity) {
	puts("ADS1115_set_comparator_polarity");
	ui8_config_register_comparator_polarity_mask = polarity;
	printf("comparator polarity mask set to: %d\n", ui8_config_register_comparator_polarity_mask);
	}
	
void ADS1115_set_comparator_latch(enum COMPARATOR_LATCH_MASK latch) {
	puts("ADS1115_set_comparator_latch");
	ui8_config_register_comparator_latch_mask = latch;
	printf("comparator latch mask set to: %d\n", ui8_config_register_comparator_latch_mask);
	}
void ADS1115_set_comparator_queue(enum COMPARATOR_QUEUE_MASK queue) {
	puts("ADS1115_set_comparator_queue");
	ui8_config_register_comparator_queue_mask = queue;
	printf("comparator queue mask set to: %d\n", ui8_config_register_comparator_queue_mask);
	}
	
void ADS1115_set_pga(enum PGA_MASK pga) {
	puts("ADS1115_set_pga");
	float f_temp_range;
	
	switch(pga) {
		case PGA_6_144:
			f_temp_range = PGA_6_144V;			
			break;
		case PGA_4_096:
			f_temp_range = PGA_4_096V;			
			break;
		case PGA_2_048:
			f_temp_range = PGA_2_048V;			
			break;
		case PGA_1_024:
			f_temp_range = PGA_1_024V;			
			break;
		case PGA_0_512:
			f_temp_range = PGA_0_512V;			
			break;
		case PGA_0_256:
			f_temp_range = PGA_0_256V;			
			break;
		default:
			f_temp_range = PGA_4_096V;			
			break;
			}
			
	f_resolution = (f_temp_range / 32768.0);
	ui8_config_register_pga_mask = pga;
	printf("pga mask set to: %d\n", ui8_config_register_pga_mask);
	}
	
double ADS1115_get_single_conversion() {	
	//puts("ADS1115_get_single_conversion");
	// set pointer register to config mode
	ui8arr_write_buffer[0] = POINTER_REGISTER_CONFIG;
	
	// clear previous register masks 
	ui8arr_write_buffer[1] = 0; // clear bits 15-8
	ui8arr_write_buffer[2] = 0; // clear bits 7-0	
	
	// apply config register masks
	ui8arr_write_buffer[1] |= ui8_config_register_operation_mask; // bit 15
	ui8arr_write_buffer[1] |= ui8_config_register_mult_mask; // bit 14-12
	ui8arr_write_buffer[1] |= ui8_config_register_pga_mask; // bit 11-9
	ui8arr_write_buffer[1] |= CONFIG_REGISTER_SINGLE_CONVERSION; // bit 8 -- force single conversion
	ui8arr_write_buffer[2] |= ui8_config_register_conversion_rate_mask; // bit 7-5
	ui8arr_write_buffer[2] |= ui8_config_register_comparator_mode_mask; // bit 4
	ui8arr_write_buffer[2] |= ui8_config_register_comparator_polarity_mask; // bit 3
	ui8arr_write_buffer[2] |= ui8_config_register_comparator_latch_mask; // bit 2
	ui8arr_write_buffer[2] |= ui8_config_register_comparator_queue_mask; // bit 1-0	
	
	// clear read buffer
	ui8arr_read_buffer[0] = 0;
	ui8arr_read_buffer[1] = 0;
	
	// write configure register
	write(i_ADS1115_handle, ui8arr_write_buffer, 3); 
	
	// wait for "done" bit to raise
	// TODO: add handle to prevent infinite loop
	while((ui8arr_read_buffer[0] & 0x80) == 0 ) { 
		read(i_ADS1115_handle, ui8arr_read_buffer, 2);
		}
	
	// set pointer register to conversion mode
	ui8arr_write_buffer[0] = 0;
	
	// start conversion
	write(i_ADS1115_handle, ui8arr_write_buffer, 3);
	
	// read conversion 
	read(i_ADS1115_handle, ui8arr_read_buffer, 2); 
	
	// stream conversion into a 16 bit field
	uint16_t ui16_temp_read_buffer = ui8arr_read_buffer[0] << 8 | ui8arr_read_buffer[1]; 
	
	// handle sign
	int i_temp_sign = 1;
	if (ui16_temp_read_buffer > 32768) {
		ui16_temp_read_buffer = (~ui16_temp_read_buffer + 1); // two's compliment
		i_temp_sign = -1;
		}
	// convert to volts
	double d_temp_conversion = (ui16_temp_read_buffer * f_resolution * i_temp_sign);
	printf("conversion = %1.4f (V)\n", d_temp_conversion);	// Print the result to terminal, first convert from binary value to mV
	return d_temp_conversion;
	}
	
double ADS1115_get_average_conversions(int count) {
	puts("ADS1115_average_conversions");
	double d_temp_average = 0;
	double d_temp_sum = 0;
	for (int c = 0; c < count; c++) {
		d_temp_sum += ADS1115_get_single_conversion();
		}
	d_temp_average = d_temp_sum / count;
	printf("average = %1.4lf (%d conversions)\n", d_temp_average, count);
	return d_temp_average;
	}
	
void ADS1115_init(int id, enum ADS1115_ADDRESS addr) {
	puts("ADS1115_init");	
	char carr_temp_I2C_name[15] = {0};	
	
	// validate address
	if (addr < 0x48) {
		addr = ADDRESS_GND;
		} 
	if (addr > 0x4B) {
		addr = ADDRESS_SCL;
		}
	
	// validate device id
	if (id < 0) {
		id = 0;
		} 
	if (id > 255) {
		id = 255; 
		}		
	
	// open i2c data stream	
	sprintf(carr_temp_I2C_name, "/dev/i2c-%i", id);
	i_ADS1115_handle = open(carr_temp_I2C_name, O_RDWR);
	if (i_ADS1115_handle < 0) {
		perror("ADS1115_init - open");
		} 			
	
	// set I2C slave
	if (ioctl(i_ADS1115_handle, I2C_SLAVE, addr) < 0) {
		perror("ADS1115_init - ioctl I2CSLAVE connect");	
		} 	
		
	printf("ADS1115_init - Successfully opened %s @ 0x%02X as I2C_SLAVE\n", carr_temp_I2C_name, addr);	
	}	
