#ifndef NUBAJA_PROJ_VARS_H_
#define NUBAJA_PROJ_VARS_H_

#define DAQ_TIMER_GROUP       TIMER_GROUP_0  // group of daq timer
#define DAQ_TIMER_IDX         0              // index of daq timer
#define DAQ_TIMER_HZ          1000           // frequency of the daq timer in Hz
#define DAQ_TIMER_DIVIDER     100
#define BSIZE                 10000
#define LAUNCH_THRESHOLD      50 //% of throttle needed for launch

//adc scales, offsets (y = scale*x + offset)
#define I_BRAKE_SCALE         1
#define I_BRAKE_OFFSET        0
#define I_BRAKE_MAX           3.6

//globals
xQueueHandle daq_timer_queue; // queue to time the daq task
xQueueHandle logging_queue_1, logging_queue_2, current_dp_queue; // queues to store data points
pid_ctrl_t brake_current_pid;

#endif
