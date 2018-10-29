#ifndef NUBAJA_PID_H_
#define NUBAJA_PID_H_

struct pid_controller 
{
	// measurements and input / output variables
	float error; 
	float old_error;
	float output; 
	
	// tuning parameters
	float kp;
	float ki;
	float kd; 

	float P;
	float I;
	float D;

	//bounds
	float windupGuard; 
	float outputMax;


};
typedef struct pid_controller *pid_t;


void pid_update ( pid_t pid, int sp, int pv )
{
	pid->old_error = pid->error; 
	pid->error = sp - pv;

	pid->P = pid->error;
	pid->I += pid->error;
	pid->D = pid->error - pid->old_error;

	if ( pid->I > pid->windupGuard ) 
	{
		pid->I = pid->windupGuard;
	}

	pid->output = ( pid->kp * pid->P ) + ( pid->ki * pid->I ) + ( pid->kd * pid->D );

	if ( pid->output > pid->outputMax ) 
	{
		pid->output = pid->outputMax;
	}	

}

// // USE - HOW THIS WOULD BE IMPLEMENTED TO CONTROL THROTTLE ACTUATOR
// int RPM; 
// int sp; 
// pid_t throttle; 

// // EACH LOOP ITERATION: 
// sp = fetch_sp(RPM);
// pid_update(throttle, sp, RPM);
// set_throttle(throttle->output);

#endif
