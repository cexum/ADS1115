/* Developer:	Charles DeGrapharee Exum
 * Date:		2018.10.12
 * Contact:		charlesexumdev@gmail.com
 * Purpose: 	interface for Texas Instruments ADS1115 adc chip
 * 
 * Preface:		add1115.h - interface for the Texas Instrument ADS1115
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
 * Written on:	Raspbian GNU/Linux 8 (jessie)
 * 				gcc (Raspbian) 4.9.2 * 
 * 
 * |||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||
 * Register Documentation from datasheet :
 *
 * >>> Pointer Register - 8 bit write only register that defines what type
 * of communication is taking place.
 * 	 bits 7:2 = reserved/unused, set to 0
 * 	 bits 1:0 = addressing :
 * 		00 = conversion register
 * 		01 = config register
 * 		10 = Lo_thresh register
 * 		11 = Hi_thresh register
 *
 * >>> Config Register - 16 bit read/write register containing several
 * configuration bit fields that control operations on the chip.
 *
 * bit 15 = Operational status or single-shot conversion start
 * 		This bit determines the operational status of the device. OS can
 * 		only be written when in power-down state and has no effect when
 * 		a conversion is ongoing.
 * 		When writing:
 * 		0 : No effect
 *		1 : Start a single conversion (when in power-down state)
 *		When reading:
 *		0 : Device is currently performing a conversion
 *		1 : Device is not currently performing a conversion
 * bits 14:12 = input multiplexer
 *		000 : AINP = AIN0 and AINN = AIN1 (default)
 *		001 : AINP = AIN0 and AINN = AIN3
 *		010 : AINP = AIN1 and AINN = AIN3
 *		011 : AINP = AIN2 and AINN = AIN3
 *		100 : AINP = AIN0 and AINN = GND
 *		101 : AINP = AIN1 and AINN = GND
 *		110 : AINP = AIN2 and AINN = GND
 *		111 : AINP = AIN3 and AINN = GND
 * bits 11:9 =  PGA config
 *		000 : FSR = ±6.144 V
 *		001 : FSR = ±4.096 V
 *		010 : FSR = ±2.048 V (default)
 *		011 : FSR = ±1.024 V
 *		100 : FSR = ±0.512 V
 *		101 : FSR = ±0.256 V
 *		110 : FSR = ±0.256 V
 *		111 : FSR = ±0.256 V
 * bit 8 = mode
 * 		0 : continous conversion
 * 		1 : single-shot or power down state
 * bits 7:5 = data rate
 * 		000 : 8 SPS
 *		001 : 16 SPS
 *		010 : 32 SPS
 *		011 : 64 SPS
 *		100 : 128 SPS (default)
 *		101 : 250 SPS
 *		110 : 475 SPS
 *		111 : 860 SPS
 * bit 4 = Comparator mode
 * 		0 : Traditional comparator (default)
 *		1 : Window comparator
 * bit 3 = Comparator polarity
 *		0 : Active low (default)
 *		1 : Active high
 * bit 2 = Comparator latch
 * 		This bit controls whether the ALERT/RDY pin latches after being asserted or
 * 		clears after conversions are within the margin of the upper and lower threshold
 * 		values. This bit serves no function on the ADS1113.
 * 		0 : Nonlatching comparator . The ALERT/RDY pin does not latch when asserted (default).
 *		1 : Latching comparator. The asserted ALERT/RDY pin remains latched until
 *		conversion data are read by the master or an appropriate SMBus alert response
 *		is sent by the master. The device responds with its address, and it is the lowest
 * 		address currently asserting the ALERT/RDY bus line.
 * bits 1:0 = Comparator queue and disable
 *		These bits perform two functions. When set to 11, the comparator is disabled and
 *		the ALERT/RDY pin is set to a high-impedance state. When set to any other
 *		value, the ALERT/RDY pin and the comparator function are enabled, and the set
 *		value determines the number of successive conversions exceeding the upper or
 *		lower threshold required before asserting the ALERT/RDY pin. These bits serve
 *		no function on the ADS1113.
 *		00 : Assert after one conversion
 *		01 : Assert after two conversions
 *		10 : Assert after four conversions
 *		11 : Disable comparator and set ALERT/RDY pin to high-impedance (default) 
 *||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||| */ 
 
//IMPORT****************************************************************
#include <stdio.h>			// perror(), printf() family, ect.
#include <inttypes.h>		// uint8_t, ect.
// i2c required headers
#include <unistd.h>			// read(), write(), usleep()
#include <fcntl.h>			// filecontrol - open()
#include <linux/i2c-dev.h> 	// I2C bus definitions
#include <sys/ioctl.h>		// ioctl()
//**********************************************************************
//DEFINE****************************************************************
#define ADDRESS_SDA		0b1001010
#define ADDRESS_SCL 	0b1001011
#define ADDRESS_GND 	0b1001000
#define ADDRESS_VDD 	0b1001001

enum ADS1115_ADDRESS {
	SDA = ADDRESS_SDA,
	SCL = ADDRESS_SCL,
	GND = ADDRESS_GND,
	VDD = ADDRESS_VDD
	};

//**********************************************************************
//**********************************************************************
/* >>> Pointer Register - 8 bit write only register that defines what type
 * of communication is taking place.
 * 	 bits 7:2 = reserved/unused, set to 0
 * 	 bits 1:0 = addressing :
 * 		00 = conversion register
 * 		01 = config register
 * 		10 = Lo_thresh register
 * 		11 = Hi_thresh register
 *  */ 
#define POINTER_REGISTER_CONVERSION 		0b00000000
#define POINTER_REGISTER_CONFIG 			0b00000001
#define POINTER_REGISTER_LOW_THRESHOLD		0b00000010
#define POINTER_REGISTER_HIGH_THRESHOLD	 	0b00000011

enum POINTER_MASK {
	CONVERSION = POINTER_REGISTER_CONVERSION,
	CONFIG = POINTER_REGISTER_CONFIG,
	LOW_THRESHOLD = POINTER_REGISTER_LOW_THRESHOLD,
	HIGH_THRESHOLD = POINTER_REGISTER_HIGH_THRESHOLD
	};
//**********************************************************************
//**********************************************************************
 /* >>> Config Register - 16 bit read/write register containing several
 * configuration bit fields that control operations on the chip.
 *
 * bit 15 = Operational status or single-shot conversion start
 * 		This bit determines the operational status of the device. OS can
 * 		only be written when in power-down state and has no effect when
 * 		a conversion is ongoing.
 * 		When writing:
 * 		0 : No effect
 *		1 : Start a single conversion (when in power-down state)
 *		When reading:
 *		0 : Device is currently performing a conversion
 *		1 : Device is not currently performing a conversion * 
 * */   
#define CONFIG_REGISTER_BUSY 							0b00000000
#define CONFIG_REGISTER_IDLE 							0b10000000
#define CONFIG_REGISTER_START_CONVERSION 		0b10000000

enum OPERATION_MASK {
	BUSY = CONFIG_REGISTER_BUSY,
	IDLE = CONFIG_REGISTER_IDLE,
	START_SINGLE_CONVERSION = CONFIG_REGISTER_START_CONVERSION 
	};
 /* * bits 14:12 = input multiplexer
 *		000 : AINP = AIN0 and AINN = AIN1 (default)
 *		001 : AINP = AIN0 and AINN = AIN3
 *		010 : AINP = AIN1 and AINN = AIN3
 *		011 : AINP = AIN2 and AINN = AIN3
 *		100 : AINP = AIN0 and AINN = GND
 *		101 : AINP = AIN1 and AINN = GND
 *		110 : AINP = AIN2 and AINN = GND
 *		111 : AINP = AIN3 and AINN = GND
 */ 
#define CONFIG_REGISTER_MULT_AIN_0_AND_1_DIFFERENTIAL 		0b00000000
#define CONFIG_REGISTER_MULT_AIN_0_AND_3_DIFFERENTIAL 		0b00010000
#define CONFIG_REGISTER_MULT_AIN_1_AND_3_DIFFERENTIAL		0b00100000
#define CONFIG_REGISTER_MULT_AIN_2_AND_3_DIFFERENTIAL 		0b00110000
#define CONFIG_REGISTER_MULT_AIN_0 							0b01000000
#define CONFIG_REGISTER_MULT_AIN_1 							0b01010000
#define CONFIG_REGISTER_MULT_AIN_2 							0b01100000
#define CONFIG_REGISTER_MULT_AIN_3 							0b01110000

enum MULT_MASK {
	MULT_AIN_0_AND_1_DIFFERENTIAL = CONFIG_REGISTER_MULT_AIN_0_AND_1_DIFFERENTIAL,
	MULT_AIN_0_AND_3_DIFFERENTIAL = CONFIG_REGISTER_MULT_AIN_0_AND_3_DIFFERENTIAL,
	MULT_AIN_1_AND_3_DIFFERENTIAL = CONFIG_REGISTER_MULT_AIN_1_AND_3_DIFFERENTIAL,
	MULT_AIN_2_AND_3_DIFFERENTIAL = CONFIG_REGISTER_MULT_AIN_2_AND_3_DIFFERENTIAL,
	MULT_AIN_0 = CONFIG_REGISTER_MULT_AIN_0,
	MULT_AIN_1 = CONFIG_REGISTER_MULT_AIN_1,
	MULT_AIN_2 = CONFIG_REGISTER_MULT_AIN_2,
	MULT_AIN_3 = CONFIG_REGISTER_MULT_AIN_3 
	};
 /* * bits 11:9 =  PGA config
 *		000 : FSR = ±6.144 V
 *		001 : FSR = ±4.096 V
 *		010 : FSR = ±2.048 V (default)
 *		011 : FSR = ±1.024 V
 *		100 : FSR = ±0.512 V
 *		101 : FSR = ±0.256 V
 *		110 : FSR = ±0.256 V
 *		111 : FSR = ±0.256 V
 */ 
#define CONFIG_REGISTER_PGA_6_144 		0b00000000
#define CONFIG_REGISTER_PGA_4_096 		0b00000010
#define CONFIG_REGISTER_PGA_2_048 		0b00000100
#define CONFIG_REGISTER_PGA_1_024 		0b00000110
#define CONFIG_REGISTER_PGA_0_512 		0b00001000
#define CONFIG_REGISTER_PGA_0_256 		0b00001010
#define PGA_6_144V 		6.144  
#define PGA_4_096V 		4.096
#define PGA_2_048V 		2.048
#define PGA_1_024V 		1.024
#define PGA_0_512V 		0.512
#define PGA_0_256V 		0.256

enum PGA_MASK {
	PGA_6_144 = CONFIG_REGISTER_PGA_6_144,
	PGA_4_096 = CONFIG_REGISTER_PGA_4_096,
	PGA_2_048 = CONFIG_REGISTER_PGA_2_048,
	PGA_1_024 = CONFIG_REGISTER_PGA_1_024,
	PGA_0_512 = CONFIG_REGISTER_PGA_0_512,
	PGA_0_256 = CONFIG_REGISTER_PGA_0_256
	};
/* * bit 8 = mode
 * 		0 : continous conversion
 * 		1 : single-shot or power down state 
 * */ 
#define CONFIG_REGISTER_CONTINUOUS_CONVERSION			0b00000000
#define CONFIG_REGISTER_SINGLE_CONVERSION				0b00000001
#define CONFIG_REGISTER_POWER_DOWN_STATE				0b00000001

enum CONVERSION_MODE_MASK {
	CONTINUOUS = CONFIG_REGISTER_CONTINUOUS_CONVERSION,
	SINGLE = CONFIG_REGISTER_SINGLE_CONVERSION,
	POWER_DOWN_STATE = CONFIG_REGISTER_POWER_DOWN_STATE
	};
/*bits 7:5 = data rate
 * 		000 : 8 SPS
 *		001 : 16 SPS
 *		010 : 32 SPS
 *		011 : 64 SPS
 *		100 : 128 SPS (default)
 *		101 : 250 SPS
 *		110 : 475 SPS
 *		111 : 860 SPS
 * */				
#define CONFIG_REGISTER_SPS_8		0b00000000
#define CONFIG_REGISTER_SPS_16 		0b00100000
#define CONFIG_REGISTER_SPS_32  	0b01000000
#define CONFIG_REGISTER_SPS_64  	0b01100000
#define CONFIG_REGISTER_SPS_128 	0b10000000
#define CONFIG_REGISTER_SPS_250 	0b10100000
#define CONFIG_REGISTER_SPS_475 	0b11000000
#define CONFIG_REGISTER_SPS_860 	0b11100000

enum RATE_MASK {
	SPS_8 = CONFIG_REGISTER_SPS_8,
	SPS_16 = CONFIG_REGISTER_SPS_16,
	SPS_32 = CONFIG_REGISTER_SPS_32,
	SPS_64 = CONFIG_REGISTER_SPS_64,
	SPS_128 = CONFIG_REGISTER_SPS_128,
	SPS_250 = CONFIG_REGISTER_SPS_250,
	SPS_475 = CONFIG_REGISTER_SPS_475,
	SPS_860 = CONFIG_REGISTER_SPS_860
	};

 /* * bit 4 = Comparator mode
 * 		0 : Traditional comparator (default)
 *		1 : Window comparator
 * */ 
#define CONFIG_REGISTER_COMPARATOR_MODE_TRADITIONAL		0b00000000
#define CONFIG_REGISTER_COMPARATOR_MODE_WINDOW			0b00010000

enum COMPARATOR_MODE_MASK {
	TRADITIONAL = CONFIG_REGISTER_COMPARATOR_MODE_TRADITIONAL,
	WINDOW = CONFIG_REGISTER_COMPARATOR_MODE_WINDOW
	};
 /* * bit 3 = Comparator polarity
 *		0 : Active low (default)
 *		1 : Active high
 * */
#define CONFIG_REGISTER_COMPARATOR_POLARITY_LOW			0b00000000
#define CONFIG_REGISTER_COMPARATOR_POLARITY_HIGH		0b00001000

enum COMPARATOR_POLARITY_MASK {
	LOW = CONFIG_REGISTER_COMPARATOR_POLARITY_LOW,
	HIGH = CONFIG_REGISTER_COMPARATOR_POLARITY_HIGH
	};
 /* * bit 2 = Comparator latch
 * 		This bit controls whether the ALERT/RDY pin latches after being asserted or
 * 		clears after conversions are within the margin of the upper and lower threshold
 * 		values. This bit serves no function on the ADS1113.
 * 		0 : Nonlatching comparator . The ALERT/RDY pin does not latch when asserted (default).
 *		1 : Latching comparator. The asserted ALERT/RDY pin remains latched until
 *		conversion data are read by the master or an appropriate SMBus alert response
 *		is sent by the master. The device responds with its address, and it is the lowest
 * 		address currently asserting the ALERT/RDY bus line.
 * */
#define CONFIG_REGISTER_COMPARATOR_NON_LATCHING		0b00000000
#define CONFIG_REGISTER_COMPARATOR_LATCHING			0b00000100

enum COMPARATOR_LATCH_MASK {
	NON_LATCHING = CONFIG_REGISTER_COMPARATOR_NON_LATCHING,
	LATCHING = CONFIG_REGISTER_COMPARATOR_LATCHING
	};
 /* * bits 1:0 = Comparator queue and disable
 *		These bits perform two functions. When set to 11, the comparator is disabled and
 *		the ALERT/RDY pin is set to a high-impedance state. When set to any other
 *		value, the ALERT/RDY pin and the comparator function are enabled, and the set
 *		value determines the number of successive conversions exceeding the upper or
 *		lower threshold required before asserting the ALERT/RDY pin. These bits serve
 *		no function on the ADS1113.
 *		00 : Assert after one conversion
 *		01 : Assert after two conversions
 *		10 : Assert after four conversions
 *		11 : Disable comparator and set ALERT/RDY pin to high-impedance (default)
 * */
#define CONFIG_REGISTER_COMPARATOR_QUEUE_LENGTH_1 	0b00000000	
#define CONFIG_REGISTER_COMPARATOR_QUEUE_LENGTH_2	0b00000001
#define CONFIG_REGISTER_COMPARATOR_QUEUE_LENGTH_4	0b00000010
#define CONFIG_REGISTER_COMPARATOR_QUEUE_DISABLED	0b00000011

enum COMPARATOR_QUEUE_MASK {
	QUEUE_LENGTH_1 = CONFIG_REGISTER_COMPARATOR_QUEUE_LENGTH_1,
	QUEUE_LENGTH_2 = CONFIG_REGISTER_COMPARATOR_QUEUE_LENGTH_2,
	QUEUE_LENGTH_4 = CONFIG_REGISTER_COMPARATOR_QUEUE_LENGTH_4,
	QUEUE_DISABLED = CONFIG_REGISTER_COMPARATOR_QUEUE_DISABLED
 	};
//**********************************************************************
//PROTOTYPE*************************************************************
void ADS1115_init(int id, enum ADS1115_ADDRESS addr);
void ADS1115_close();
void ADS1115_set_conversion_rate(enum RATE_MASK sps);
void ADS1115_set_conversion_mode(enum CONVERSION_MODE_MASK );
void ADS1115_set_multiplex(enum MULT_MASK mult);
void ADS1115_set_comparator_mode(enum COMPARATOR_MODE_MASK cmm);
void ADS1115_set_comparator_polarity(enum COMPARATOR_POLARITY_MASK cpm);
void ADS1115_set_comparator_latch(enum COMPARATOR_LATCH_MASK clm);
void ADS1115_set_comparator_queue(enum COMPARATOR_QUEUE_MASK);
void ADS1115_set_pga(enum PGA_MASK pga);
double ADS1115_get_single_conversion();
double ADS1115_get_average_conversions(int count);
//**********************************************************************
