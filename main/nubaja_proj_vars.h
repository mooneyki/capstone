#ifndef NUBAJA_PROJ_VARS_H_
#define NUBAJA_PROJ_VARS_H_

//timing
#define DAQ_TIMER_GROUP       	TIMER_GROUP_0  // group of daq timer
#define DAQ_TIMER_IDX         	0              // index of daq timer
#define DAQ_TIMER_HZ          	10           // frequency of the daq timer in Hz
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
	int en_log;

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

float i_sp_test[BSIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0-.9
						   0, 100, 100, 99, 99, 99, 99, 98, 98, 98, //1-1.9
						   97, 97, 97, 96, 96, 96, 95, 95, 95, 94,
						   94, 94, 93, 93, 92, 92, 91, 91, 91, 90,
						   90, 89, 89, 88, 87, 87, 86, 86, 85, 85,
						   84, 83, 83, 82, 81, 81, 80, 79, 78, 78,
						   77, 76, 75, 74, 74, 73, 72, 71, 70, 69,
						   68, 67, 67, 66, 65, 64, 63, 62, 61, 60,
						   59, 58, 57, 56, 55, 54, 52, 51, 50, 49,
						   48, 47, 46, 45, 44, 42, 41, 40, 39, 38};

float tps_sp_test[BSIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						   10, 10, 10, 10, 10, 25, 25, 25, 25, 25,
						   35, 35, 35, 35, 35, 35, 35, 35, 35, 35,
						   50, 50, 50, 50, 50, 50, 50, 50, 50, 50,
						   100, 100, 100, 100, 100, 100, 100, 100, 100, 100,
						   40, 40, 40, 40, 40, 40, 40, 40, 40, 40,
						   60, 60, 60, 60, 60, 60, 60, 60, 60, 60,
						   30, 30, 30, 30, 30, 30, 30, 30, 30, 30,
						   15, 15, 15, 15, 15, 15, 15, 15, 15, 15,
						   0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

float i_sp_demo[BSIZE] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						   0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
						   0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

float tps_sp_demo[BSIZE] = { 0, 2, 4, , 6, 8, 10, 12, 14,16,
						   18, 20, 22, 24, 26, 28, 30, 32, 34, 36,
						   38, 40, 42, 44, 46, 48, 50, 52, 54, 56,
						   58, 60, 62, 64, 66, 68, 70, 72, 74, 76,
						   78, 80, 82, 84, 86, 88, 90, 92, 94, 96,
						   98, 100, 98, 96, 94, 92,90, 88, 86, 84,
						   82, 80, 78, 76, 74, 72, 70, 68, 66, 64,
						   62, 60, 58, 56, 54, 52, 50, 48, 46, 44,
						   42, 40, 36, 32, 28, 24, 20, 18, 16, 14,
						   12, 10, 8, 6, 4, 2, 0, 0, 0, 0};

#endif
