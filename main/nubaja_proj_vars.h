#ifndef NUBAJA_PROJ_VARS_H_
#define NUBAJA_PROJ_VARS_H_

//timing
#define DAQ_TIMER_GROUP       	TIMER_GROUP_0  // group of daq timer
#define DAQ_TIMER_IDX         	0              // index of daq timer
#define DAQ_TIMER_HZ          	1           // frequency of the daq timer in Hz
#define DAQ_TIMER_DIVIDER     	100
#define BSIZE                 	10000

//ctrl
#define LAUNCH_THRESHOLD      	50 //% of throttle needed for launch

//adc scales, offsets (y = scale*x + offset)
#define I_BRAKE_SCALE         	1
#define I_BRAKE_OFFSET        	0.05
#define I_BRAKE_MAX           	3.6

//fault thresholds
#define MAX_I_BRAKE				2.4 //amps
#define MAX_BELT_TEMP			200 //deg F
#define MAX_CVT_AMBIENT 		200 //deg F

//struct for consolidating various flags, key quantities, etc
struct control 
{
	//flags
	int en_eng; 
	int eng;
	int run;
	int num_profile;
	int idx;

	//quantities
	float i_brake_amps; 
	float i_brake_duty; 
};
typedef struct control control_t;

#endif
