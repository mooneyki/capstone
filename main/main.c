#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#include "nubaja_proj_vars.h"
#include "nubaja_gpio.h"
#include "nubaja_fault.h"
#include "nubaja_i2c.h"
#include "nubaja_ad7998.h"
#include "nubaja_sd.h"
#include "nubaja_pid.h"
#include "nubaja_pwm.h"

//globals
xQueueHandle daq_timer_queue; // queue to time the daq task
xQueueHandle logging_queue_1, logging_queue_2, current_dp_queue; // queues to store data points
pid_ctrl_t brake_current_pid;
fault_t ctrl_faults; 
control_t main_ctrl;
float i_sp[BSIZE]; //brake current set point array (0-100%)
float tps_sp[BSIZE]; //throttle position set point array (0-100%)

// interrupt for daq_task timer
void IRAM_ATTR daq_timer_isr( void *para )
{
  // retrieve the interrupt status and the counter value from the timer
  uint32_t intr_status = TIMERG0.int_st_timers.val;
  TIMERG0.hw_timer[DAQ_TIMER_IDX].update = 1;

  // clear the interrupt
  if ( intr_status & BIT(DAQ_TIMER_IDX) )
  {
    TIMERG0.int_clr_timers.t0 = 1;
  }

  // enable the alarm again, so it is triggered the next time
  TIMERG0.hw_timer[DAQ_TIMER_IDX].config.alarm_en = TIMER_ALARM_EN;

  // send the counter value to the queue to trigger the daq task
  xQueueOverwriteFromISR( daq_timer_queue, &intr_status, NULL );
}

static void daq_timer_init()
{
  // select and initialize basic parameters of the timer
  timer_config_t config;
  config.divider = DAQ_TIMER_DIVIDER;
  config.counter_dir = TIMER_COUNT_UP;
  config.counter_en = TIMER_PAUSE;
  config.alarm_en = TIMER_ALARM_EN;
  config.intr_type = TIMER_INTR_LEVEL;
  config.auto_reload = TIMER_AUTORELOAD_EN;
  timer_init(DAQ_TIMER_GROUP, DAQ_TIMER_IDX, &config);

  // timer's counter will initially start from value below
  timer_set_counter_value( DAQ_TIMER_GROUP, DAQ_TIMER_IDX, 0x00000000ULL );

  // configure the alarm value and the interrupt on alarm
  timer_set_alarm_value( DAQ_TIMER_GROUP, DAQ_TIMER_IDX, TIMER_BASE_CLK / DAQ_TIMER_DIVIDER / DAQ_TIMER_HZ );
  timer_enable_intr( DAQ_TIMER_GROUP, DAQ_TIMER_IDX );
  timer_isr_register( DAQ_TIMER_GROUP, DAQ_TIMER_IDX, daq_timer_isr, NULL, ESP_INTR_FLAG_IRAM, NULL );

  timer_start(DAQ_TIMER_GROUP, DAQ_TIMER_IDX);
}

static void get_profile () 
{
  int i = 0;

  //choose test
  printf("Test selection. Enter profile number.\n");
  printf("Profile 1 - acceleration w/ launch.\n");
  printf("Profile 2 - acceleration w/o launch.\n");
  printf("Profile 3 - hill climb.\n");
  printf("Profile 4 - test.\n");
  printf("Profile 5 - demo.\n");
  while ( !main_ctrl.num_profile ) {
    scanf("%d", &main_ctrl.num_profile);
  }

  switch( main_ctrl.num_profile ) 
  {
    case 1:
      for ( i = 0; i < BSIZE; i++) {
        i_sp[i] = i_sp_accel_launch[i];
        tps_sp[i] = tps_sp_accel_launch[i];
      }
      break; 

    case 2:
      for ( i = 0; i < BSIZE; i++) {
        i_sp[i] = i_sp_accel[i];
        tps_sp[i] = tps_sp_accel[i];
      }
      break; 

    case 3:
      for ( i = 0; i < BSIZE; i++) {
        i_sp[i] = i_sp_hill[i];
        tps_sp[i] = tps_sp_hill[i];
      }   
      break;

    case 4:
      for ( i = 0; i < BSIZE; i++) {
        i_sp[i] = i_sp_test[i];
        tps_sp[i] = i_sp_demo[i]; //THROTTLE DISABLED
      } 
      break;

    case 5:
      for ( i = 0; i < BSIZE; i++) {
        i_sp[i] = i_sp_demo[i];
        tps_sp[i] = tps_sp_demo[i];
        main_ctrl.en_log = 0;
      } 
      break;      
  }

}


// task to run the main daq system based on a timer
static void daq_task(void *arg)
{

  /** INIT STAGE **/

  // vars
  uint32_t intr_status;

  //flags
  main_ctrl.en_eng = 0; 
  main_ctrl.eng = 0;
  main_ctrl.run = 1;
  main_ctrl.idx = 0;
  main_ctrl.num_profile = 0;
  main_ctrl.en_log = 1;

  //quantities
  main_ctrl.i_brake_amps = 0; 
  main_ctrl.i_brake_duty = 0; 
  main_ctrl.brake_temp = 0; 
  main_ctrl.belt_temp = 0; 

  data_point dp =
  {
    dp.prim_rpm = 0,    dp.sec_rpm = 0,        dp.torque = 0,
    dp.temp3 = 0,       dp.belt_temp = 0,      dp.temp2 = 0,
    dp.i_brake = 0,     dp.temp1 = 0,          dp.load_cell = 0, 
    dp.tps = 0,         dp.i_sp = 0,           dp.tps_sp = 0
  }; //empty data point  

  //module, peripheral configurations

  // init ADC w/ channel selection
  i2c_master_config( PORT_0, FAST_MODE_PLUS, I2C_MASTER_0_SDA_IO, I2C_MASTER_0_SCL_IO );
  // uint8_t ch_sel_h = ( CH6 );
  // uint8_t ch_sel_l = ( CH4 | CH3 | CH2 );
  uint8_t ch_sel_h = ( CH8 | CH7 | CH6 | CH5 );
  uint8_t ch_sel_l = ( CH4 | CH3 | CH2 | CH1 );  
  ad7998_config( PORT_0, ADC_SLAVE_ADDR, ch_sel_h, ch_sel_l ); 

  // // init sd
  init_sd();
  xQueueHandle current_logging_queue = logging_queue_1;

  //init GPIOs
  configure_gpio();

  //init PWMs
  pwm_init();

  //init PIDs
  init_pid( &brake_current_pid, KP, KI, KD, BRAKE_WINDUP_GUARD, BRAKE_OUTPUT_MAX );

  //init faults
  clear_faults ( &ctrl_faults );

  //default states
  ebrake_set();
  engine_on();
  flasher_off(); 
  set_throttle(0); //no throttle
  set_brake_duty(0); //no braking 

  // //prompt user to start engine
  // printf("Start engine.\n");
  // printf("1 = YES ; 0 = NO\n");
  // while ( !main_ctrl.en_eng ) 
  // {
  //   scanf("%d\n", &main_ctrl.en_eng);
  // }
  // engine_on();

  //prompt user to confirm engine running and warm
  printf("Engine running?\n");
  printf("1 = YES ; 0 = NO\n");  
  while ( !main_ctrl.eng ) {
    scanf("%d\n", &main_ctrl.eng);    
  }

  flasher_on();
  printf("\n\n\n\n\n-------------- LO0000000OP --------------\n\n\n\n\n");
  /** END INIT STAGE **/  

  /** LOOP STAGE **/

  while ( main_ctrl.run )
  {
    // wait for timer alarm
    xQueueReceive( daq_timer_queue, &intr_status, portMAX_DELAY );

    //check if test is done (profiles ended) or if test faulted
    if ( ( main_ctrl.idx == BSIZE ) | ( ctrl_faults.trip ) ) {
        // main_ctrl.run = 0;
        main_ctrl_idx = 0;
    }

    //get new set points (in the form of 0-100% i.e. duty cycle)
    dp.i_sp = fetch_sp ( main_ctrl.idx, i_sp );
    dp.tps_sp = fetch_sp ( main_ctrl.idx, tps_sp );

    //e-brake release
    if ( dp.tps_sp > LAUNCH_THRESHOLD )
    {
      ebrake_release();
    }

    //RECORD DATA
    // adc
    ad7998_read_3( PORT_0, ADC_SLAVE_ADDR, 
                  &(dp.torque),
                  &(dp.temp3),
                  &(dp.belt_temp),
                  &(dp.temp2),
                  &(dp.i_brake), 
                  &(dp.temp1), 
                  &(dp.load_cell), 
                  &(dp.tps) );

    //relevant physical quantity conversion for faults
    main_ctrl.i_brake_amps = ( counts_to_volts ( dp.i_brake ) * I_BRAKE_SCALE )  + I_BRAKE_OFFSET; //ADC counts to amps
    main_ctrl.i_brake_duty = 100 * ( main_ctrl.i_brake_amps / I_BRAKE_MAX ); //convert brake current in amps to duty cycle from 0-100%
    main_ctrl.brake_temp = ( counts_to_volts ( dp.temp3 ) * THERM_SCALE )  + THERM_OFFSET; //ADC counts to amps
    main_ctrl.belt_temp = ( counts_to_volts ( dp.belt_temp ) * BELT_TEMP_SCALE )  + BELT_TEMP_OFFSET; //ADC counts to amps
    
    // printf( "brake current: %4.2f\n", main_ctrl.i_brake_amps );

    // rpm measurements
    rpm_log ( primary_rpm_queue, &(dp.prim_rpm) );
    rpm_log ( secondary_rpm_queue, &(dp.sec_rpm) );

    //update PID
    // pid_update ( &brake_current_pid, dp.i_sp, main_ctrl.i_brake_duty );
    // printf( "brake cmd: %4.2f\n", brake_current_pid.output );

    //set brake current, throttle
    set_throttle( dp.tps_sp ); 
    // set_brake_duty( brake_current_pid.output ); 
    set_brake_duty( dp.i_sp ); 


    print_data_point( &dp );

    // check for faults
    if ( main_ctrl.i_brake_amps > MAX_I_BRAKE ) 
    {
      ctrl_faults.trip = 1;
      ctrl_faults.overcurrent_fault = 1;      
    }
    if ( ( main_ctrl.belt_temp > MAX_BELT_TEMP ) | ( main_ctrl.brake_temp > MAX_BRAKE_TEMP ) )
    {
      ctrl_faults.trip = 1;
      ctrl_faults.overtemp_fault = 1;      
    }

    // push struct to logging queue
    // if the queue is full, switch queues and send the full for writing to SD
    if ( main_ctrl.en_log )   
    {
      if (xQueueSend( current_logging_queue, &dp, 0 ) == errQUEUE_FULL )
      {
        printf("daq_task -- queue full, writing and switiching...\n");
        if ( current_logging_queue == logging_queue_1 )
        {
          current_logging_queue = logging_queue_2;
          xTaskCreatePinnedToCore( write_logging_queue_to_sd,
                  "write_lq_1_sd", 2048, (void *) logging_queue_1,
                  (configMAX_PRIORITIES-1), NULL, 1 );
        }
      else
      {
          current_logging_queue = logging_queue_1;
          xTaskCreatePinnedToCore( write_logging_queue_to_sd,
                  "write_lq_2_sd", 2048, (void *) logging_queue_2,
                  (configMAX_PRIORITIES-1), NULL, 1 );
        }

        // reset, though queue should be empty after writing
        // queue won't be empty if a writing task overlap occurred
        xQueueReset( current_logging_queue );
      }
    }

    ++main_ctrl.idx;
  }

  /** END LOOP STAGE **/
  
  //restore defaults, safe system shutdown
  set_throttle( 0 ); //no throttle
  set_brake_duty( 0 ); //no braking 
  reset_pid( &brake_current_pid );
  engine_off();
  flasher_off();
  ebrake_set();
  xTaskCreatePinnedToCore( write_logging_queue_to_sd,
                "write_lq_final_sd", 2048, (void *) current_logging_queue,
                (configMAX_PRIORITIES-1), NULL, 1 );  

  // per FreeRTOS, tasks MUST be deleted before breaking out of its implementing funciton
  vTaskDelete(NULL);
}

// initialize the daq timer and start the daq task
void app_main()
{
  daq_timer_queue = xQueueCreate(1, sizeof(uint32_t));

  logging_queue_1 = xQueueCreate( LOGGING_QUEUE_SIZE, sizeof(data_point) );
  logging_queue_2 = xQueueCreate( LOGGING_QUEUE_SIZE, sizeof(data_point) );

  // setup current data point queue
  current_dp_queue = xQueueCreate( 1, sizeof(data_point) );
  data_point dp =
  {
    dp.prim_rpm = 0,    dp.sec_rpm = 0,        dp.torque = 0,
    dp.temp3 = 0,       dp.belt_temp = 0,      dp.temp2 = 0,
    dp.i_brake = 0,     dp.temp1 = 0,          dp.load_cell = 0, 
    dp.tps = 0,         dp.i_sp = 0,           dp.tps_sp = 0
  };
  xQueueOverwrite( current_dp_queue, &dp );

  // start daq timer and tasks
  daq_timer_init();
  get_profile();

  xTaskCreatePinnedToCore( daq_task, "daq_task", 4096, NULL, (configMAX_PRIORITIES-1), NULL, 0 );
}




