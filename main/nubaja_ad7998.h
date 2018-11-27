#ifndef NUBAJA_AD7998_H_
#define NUBAJA_AD7998_H_

#include "nubaja_i2c.h"
#include "nubaja_sd.h"

//run in fast mode plus
//use cmd mode

#define ADC_SLAVE_ADDR			0x23 //pn ad7998-1 with AS @ GND. this is default address. 
#define ADC_FS					3.3

//register addresses
#define CONFIGURATION			0b01110010

//configuration register
#define ALERT_BUSY				0x0 //pin does not provide any interrupt signal
#define ALERT_EN				0x0 //pin does not provide any interrupt signal
#define FLTR 					0b1000 //filtering disabled on SDA/SCL
#define ALERT_BUSY_POLARITY		0x0 //alert/busy output is active low
#define CHANNEL_SELECTION_0		0b0000010101010000 //torque, belt temp, brake current, load cell		
#define CHANNEL_SELECTION_1		0b0000001011100000 //temps only
#define CHANNEL_SELECTION_2		0x0 //idk
#define CHANNEL_SELECTION_3		0b0000111111110000 //all channels
#define CH1 					0b00010000
#define CH2 					0b00100000
#define CH3 					0b01000000				
#define CH4 					0b10000000
#define CH5 					0b0001
#define CH6 					0b0010
#define CH7 					0b0100
#define CH8 					0b1000


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

//each new read function requires another call of ad7998_init with the corresponding channel selection

float counts_to_volts ( uint16_t adc_counts ) 
{
	float v = ( (float) adc_counts / (float) 4096 ) * (float) ADC_FS;
	return v;
}

void ad7998_config( int port_num, int slave_address, uint8_t ch_sel_h, uint8_t ch_sel_l ) 
{
	uint8_t addr_ptr = CONFIGURATION; 
	uint8_t data_h = ch_sel_h;
	uint8_t data_l = ( ch_sel_l | FLTR | ALERT_EN | ALERT_BUSY | ALERT_BUSY_POLARITY );
	i2c_write_2_byte(port_num, slave_address, addr_ptr, data_h, data_l );
}

//read function for channel selection 0
void ad7998_read_0 ( int port_num, int slave_address, 
	uint16_t *ch1, 
	uint16_t *ch3,
	uint16_t *ch5,
	uint16_t *ch7 )
{
	uint8_t addr_pointer = CMD_MODE; //cmd mode
	i2c_read_2_bytes_4( port_num, slave_address, addr_pointer, 
		ch1, ch3, ch5, ch7 );
}

//read function for channel selection 1
void ad7998_read_1 ( int port_num, int slave_address, 
	uint16_t *ch2, 
	uint16_t *ch3,
	uint16_t *ch4,
	uint16_t *ch6 )
{
	uint8_t addr_pointer = CMD_MODE; //cmd mode
	i2c_read_2_bytes_4( port_num, slave_address, addr_pointer, 
		ch2, ch3, ch4, ch6 );
}

//read function for channel selection 3
void ad7998_read_3 ( int port_num, int slave_address, 
	uint16_t *ch1, 
	uint16_t *ch2,
	uint16_t *ch3,
	uint16_t *ch4, 
	uint16_t *ch5, 
	uint16_t *ch6,
	uint16_t *ch7,
	uint16_t *ch8 )
{
	uint8_t addr_pointer = CMD_MODE; //cmd mode
	
	i2c_read_2_bytes_8( port_num, slave_address, addr_pointer, 
		 ch1,  ch2,  ch3,  ch4,
		 ch5,  ch6,  ch7,  ch8 );
}

#endif