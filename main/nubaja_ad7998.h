#ifndef NUBAJA_AD7998_H_
#define NUBAJA_AD7998_H_

#include "nubaja_i2c.h"

//run in fast mode plus
//use cmd mode

#define ADC_SLAVE_ADDR		0x23 //pn ad7998-1 with AS @ GND. this is default address. 

//register addresses
#define CONVERSION_RESULT		0x0
#define ALERT_STATUS			0x1
#define CONFIGURATION			0x2
#define CYCLE_TIMER				0x3

//configuration register
#define ALERT_BUSY				0x0 //pin does not provide any interrupt signal
#define FLTR 					0b1000 //filtering disabled on SDA/SCL
#define ALERT_BUSY_POLARITY		0x0 //alert/busy output is active low
#define CHANNEL_SELECTION_0		0b0000010101010000 //torque, belt temp, brake current, load cell		
#define CHANNEL_SELECTION_1		0b0000001011100000 //temps only
#define CHANNEL_SELECTION_2		0x0 //idk
#define CHANNEL_SELECTION_3		0b0000111111110000 //all channels

//cycle timer register
#define CYCLE_TIME 				0b00000100 //0.5ms conversion interval 
#define SAMPLE_DELAY_TRIAL		0b11000000 //bit trial and sample interval delaying mechanism implemented

//command mode
#define CMD_MODE 				0b01110000 //sequence of channels specified in the config register

/*
** CHANNEL MAPPING - MAPS ADC CHANNELS TO SIGNAL/NET NAMES
Channel 1 - torque transducer
Channel 2 - temp 3 (tbd)
Channel 3 - belt temp
Channel 4 - temp 2 (tbd)
Channel 5 - brake current
Channel 6 - temp1 (tbd)
Channel 7 - load cell
Channel 8 - throttle position
*/

void ad7998_init( int port_num, int slave_address, int channel_selection ) 
{
	i2c_write_byte(port_num, slave_address, CONFIGURATION, ( channel_selection | FLTR | ALERT_BUSY | ALERT_BUSY_POLARITY ) );

}

//read function for channel selection 0
void ad7998_read_0 ( int port_num, int slave_address )
{
	uint8_t addr_pointer = CMD_MODE; //cmd mode
	i2c_read_2_bytes_4( port_num, slave_address, addr_pointer )
}

#endif