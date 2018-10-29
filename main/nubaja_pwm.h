#ifndef NUBAJA_PWM_H_
#define NUBAJA_PWM_H_

#include "driver/mcpwm.h"
#include "soc/mcpwm_reg.h"
#include "soc/mcpwm_struct.h"

//You can get these value from the datasheet of servo you use, in general pulse width varies between 1000 to 2000 mocrosecond
#define SERVO_MIN_PULSEWIDTH 	700 			//Minimum pulse width in microsecond
#define SERVO_MAX_PULSEWIDTH 	2300 			//Maximum pulse width in microsecond
#define SERVO_MAX_DEGREE 		120 			//Maximum angle in degree upto which servo can rotate
#define THROTTLE_PWM 			25 				//net name IO25_THROTTLE
#define BRAKE_PWM 				17 				//net name IO17_BRAKE_PWM
#define SERVO_PWM_FREQUENCY		50				//HZ
#define BRAKE_PWM_FREQUENCY		1000			//HZ
#define SERVO_OFFSET_DEG 		0

void pwm_init () 
{
	mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM0A, THROTTLE_PWM);    //Set GPIO 25 as PWM0A
	mcpwm_gpio_init(MCPWM_UNIT_0, MCPWM1A, BRAKE_PWM);    //Set GPIO 17 as PWM1A

	//servo config
    mcpwm_config_t pwm_config;
    pwm_config.frequency = SERVO_PWM_FREQUENCY;    
    pwm_config.cmpr_a = 0; //duty cycle from 0-100   
    pwm_config.cmpr_b = 0;   
    pwm_config.counter_mode = MCPWM_UP_COUNTER;
    pwm_config.duty_mode = MCPWM_DUTY_MODE_0;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_0, &pwm_config);    

    //brake drive config
    pwm_config.frequency = BRAKE_PWM_FREQUENCY;
    mcpwm_init(MCPWM_UNIT_0, MCPWM_TIMER_1, &pwm_config);    	
}

/**
 * @brief Use this function to calcute pulse width for per degree rotation
 */
static uint32_t cal_pulsewidth(uint32_t degree_of_rotation)
{
    uint32_t cal_pulsewidth = 0;
    cal_pulsewidth = (SERVO_MIN_PULSEWIDTH + (((SERVO_MAX_PULSEWIDTH - SERVO_MIN_PULSEWIDTH) * (degree_of_rotation)) / (SERVO_MAX_DEGREE)));
    return cal_pulsewidth;
}

void set_throttle (uint32_t tp) //tps from 0-100%
{
	uint32_t servo_angle = ( tps * 0.9 ) + SERVO_OFFSET_DEG; //TO-DO - DEFINE THIS RELATIONSHIP
	uint32_t pw = cal_pulsewidth( servo_angle );
	mcpwm_set_duty_in_us(MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM_OPR_A, pw);	
}

void set_brake_duty (float duty) 
{
	mcpwm_set_duty(MCPWM_UNIT_0, MCPWM_TIMER_1, MCPWM_OPR_A, duty);	
}

#endif