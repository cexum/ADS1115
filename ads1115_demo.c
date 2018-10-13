/* File:		ads1115_demo.c 
 * Developer:	Charles DeGrapharee Exum
 * Date:		2018.10.12
 * Contact:		charlesexumdev@gmail.com
 * Purpose: 	demonstrate functionality of ads1115.c
 */

#include "ads1115.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char **argv) {	
	
	ADS1115_init(1, SDA);	
	ADS1115_set_pga(PGA_4_096);
	ADS1115_set_conversion_rate(SPS_16);
	ADS1115_set_multiplex(MULT_AIN_0);
	
	double darr_data[10] = {0};
	double d_temp = 0;
	for (int c = 0; c < 10; c++) {
		darr_data[c] = ADS1115_get_average_conversions(8);		
		}		
	return 0;
	}

