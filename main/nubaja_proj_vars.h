#ifndef NUBAJA_PROJ_VARS_H_
#define NUBAJA_PROJ_VARS_H_

//timing
#define DAQ_TIMER_GROUP       	TIMER_GROUP_0  // group of daq timer
#define DAQ_TIMER_IDX         	0              // index of daq timer
#define DAQ_TIMER_HZ          	1           // frequency of the daq timer in Hz
#define DAQ_TIMER_DIVIDER     	100


//ctrl
#define LAUNCH_THRESHOLD      	50 //% of throttle needed for launch
#define BSIZE                 	100 //test length

//adc scales, offsets (y = scale*x + offset)
#define I_BRAKE_SCALE         	1
#define I_BRAKE_OFFSET        	0.05
#define I_BRAKE_MAX           	3.6

//fault thresholds
#define MAX_I_BRAKE				2.4 //amps
#define MAX_BELT_TEMP			200 //deg F
#define MAX_CVT_AMBIENT 		200 //deg F

//PIDs
#define	KP						0 
#define	KI						0.5
#define	KD						0
#define	BRAKE_WINDUP_GUARD		10
#define	BRAKE_OUTPUT_MAX		100

//struct for consolidating various flags, key quantities, etc
struct control 
{
	//flags
	int en_eng; 
	int eng;
	int run;
	int num_profile;
	int test_chosen;
	int idx;

	//quantities
	float i_brake_amps; 
	float i_brake_duty; 
};
typedef struct control control_t;

//profiles
float i_sp_accel_launch[BSIZE];
float tps_sp_accel_launch[BSIZE];
float i_sp_accel[BSIZE];
float tps_sp_accel[BSIZE];
float i_sp_hill[BSIZE];
float tps_sp_hill[BSIZE];
float i_sp_test[BSIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						   10, 10, 10, 10, 10, 25, 25, 25, 25, 25,
						   35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
						   50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
						   80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
						   40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
						   60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
						   30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
						   15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
						   0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

float tps_sp_test[BSIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						   10, 10, 10, 10, 10, 25, 25, 25, 25, 25,
						   35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
						   50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
						   80, 80, 80, 80, 80, 80, 80, 80, 80, 80,
						   40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
						   60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
						   30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
						   15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
						   0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

#endif
