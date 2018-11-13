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


// task to run the main daq system based on a timer
static void daq_task(void *arg)
{

  /** INIT STAGE **/

  // vars
  uint32_t intr_status;
  char filename_i[100];
  char filename_t[100];
  char buffer[BSIZE];
  FILE *fp;
  char *field;
  float i_sp[BSIZE]; //brake current set point array (0-100%)
  float tps_sp[BSIZE]; //throttle position set point array (0-100%)
  int i = 0;
  int j = 0;
  
  //flags
  main_ctrl.en_eng = 0; 
  main_ctrl.eng = 0;
  main_ctrl.run = 1;
  main_ctrl.idx = 0;

  //quantities
  main_ctrl.i_brake_amps = 0; 
  main_ctrl.i_brake_duty = 0; 

  data_point dp =
  {
    dp.prim_rpm = 0,    dp.sec_rpm = 0,        dp.torque = 0,
    dp.temp3 = 0,       dp.belt_temp = 0,      dp.temp2 = 0,
    dp.i_brake = 0,     dp.temp1 = 0,          dp.load_cell = 0, 
    dp.tps = 0,         dp.i_sp = 0,           dp.tps_sp = 0
  }; //empty data point  

  //module configurations
  // init ADC
  i2c_master_config( PORT_0, FAST_MODE_PLUS, I2C_MASTER_0_SDA_IO, I2C_MASTER_0_SCL_IO );
  ad7998_init( PORT_0, ADC_SLAVE_ADDR, CHANNEL_SELECTION_0 ); //init adc

  // init sd
  init_sd();
  xQueueHandle current_logging_queue = logging_queue_1;

  //init GPIOs
  configure_gpio();

  //init PIDs
  init_pid ( brake_current_pid, 0, 0.1, 0, 10, 100 );

  //init faults
  clear_faults ( ctrl_faults );

  //default states
  ebrake_set();
  engine_off();
  flasher_off(); 
  set_throttle(0); //no throttle
  set_brake_duty(0); //no braking 

  //choose test
  printf("Test selection. Enter profile number.\n");
  printf("Profile 1 - acceleration w/ launch.\n");
  printf("Profile 2 - acceleration w/o launch.\n");
  printf("Profile 3 - hill climb.\n");
  scanf("%d", &main_ctrl.num_profile);

  switch( main_ctrl.num_profile ) 
  {
    case 1:
      strcpy(filename_i, "/sdcard/profiles/brake_current_profile_1.csv");
      strcpy(filename_t, "/sdcard/profiles/throttle_profile_1.csv");
      break; 

    case 2:
      strcpy(filename_i, "/sdcard/profiles/brake_current_profile_2.csv");
      strcpy(filename_t, "/sdcard/profiles/throttle_profile_2.csv");
      break; 

    case 3:
      strcpy(filename_i, "/sdcard/profiles/brake_current_profile_3.csv");
      strcpy(filename_t, "/sdcard/profiles/throttle_profile_3.csv");
      break;
  }

  //LOAD BRAKE PROFILE FROM CSV FILE
  /* open the CSV file */
  fp = fopen(filename_i,"r");
  if( fp == NULL)
  {
    printf("Unable to open file '%s'\n",filename_i);
    exit(1);
  }

  /* parse data */
  while(fgets(buffer,BSIZE,fp))
  {
    field=strtok(buffer,","); /* get value */
    i_sp[i]=atof(field);
    
    /* display the result in the proper format */
    printf("brake current sp: %5.1f\n",
        i_sp[i]);
        
    ++i;
  }

  fclose(fp); /* close file */

  //LOAD THROTTLE PROFILE FROM CSV FILE
  /* open the CSV file */
  fp = fopen(filename_t,"r");
  if( fp == NULL)
  {
    printf("Unable to open file '%s'\n",filename_t);
    exit(1);
  }

  /* parse data */
  while(fgets(buffer,BSIZE,fp))
  {
    field=strtok(buffer,","); /* get value */
    tps_sp[j]=atof(field);
    
    /* display the result in the proper format */
    printf("throttle position sp: %5.1f\n",
        tps_sp[j]);

    ++j;
  }

  fclose(fp); /* close file */  

  if (j != i) {
    printf("Unmatched throttle and brake profiles.\n"); 
    exit(1);
  }

  //prompt user to start engine
  while ( !main_ctrl.en_eng ) 
  {
    printf("Start engine?\n");
    scanf("%d", &main_ctrl.en_eng);
  }
  engine_on();

  //prompt user to confirm engine running and warm
  while ( !main_ctrl.eng ) {
    printf("Engine running?\n");
    scanf("%d", &main_ctrl.eng);    
  }

  flasher_on();

  /** END INIT STAGE **/  




  /** LOOP STAGE **/

  while ( main_ctrl.run )
  {
    // wait for timer alarm
    xQueueReceive( daq_timer_queue, &intr_status, portMAX_DELAY );
   
    //check if test is done (profiles ended) or if test faulted
    if ( ( main_ctrl.idx == i ) | ( ctrl_faults->trip ) ) {
        main_ctrl.run = 0;
    }

    //get new set points
    dp.i_sp = fetch_sp ( main_ctrl.idx, i_sp );
    dp.tps_sp = fetch_sp ( main_ctrl.idx, tps_sp );

    //e-brake release
    if ( dp.tps_sp > LAUNCH_THRESHOLD )
    {
      ebrake_release();
    }

    //RECORD DATA
    // adc
    ad7998_read_0( PORT_0, ADC_SLAVE_ADDR, &(dp.torque), &(dp.belt_temp), &(dp.i_brake), &(dp.load_cell) );
    main_ctrl.i_brake_amps = ( counts_to_volts ( dp.i_brake ) * I_BRAKE_SCALE )  + I_BRAKE_OFFSET; //ADC counts to amps
    main_ctrl.i_brake_duty = 100 * ( main_ctrl.i_brake_amps / I_BRAKE_MAX ); //convert brake current in amps to duty cycle from 0-100%

    // rpm measurements
    xQueuePeek( primary_rpm_queue, &(dp.prim_rpm), 0 );
    xQueuePeek( secondary_rpm_queue, &(dp.sec_rpm), 0 );

    //update PIDs
    pid_update ( brake_current_pid, dp.i_sp, main_ctrl.i_brake_duty );

    //set brake current / throttle
    set_throttle( dp.tps_sp ); 
    set_brake_duty( brake_current_pid->output ); 

    //check for faults
    if ( main_ctrl.i_brake_amps > MAX_I_BRAKE ) {
      ctrl_faults->trip = 1;
      ctrl_faults->overcurrent_fault = 1;      
    }

    // push struct to logging queue
    // if the queue is full, switch queues and send the full for writing to SD
    if (xQueueSend( current_logging_queue, &dp, 0) == errQUEUE_FULL )
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

    ++main_ctrl.idx;
  }

  /** END LOOP STAGE **/
  
  //restore defaults, safe system shutdown
  set_throttle( 0 ); //no throttle
  set_brake_duty( 0 ); //no braking 
  reset_pid( brake_current_pid );
  engine_off();
  flasher_off();
  ebrake_set();

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

  xTaskCreatePinnedToCore( daq_task, "daq_task", 2048, NULL, (configMAX_PRIORITIES-1), NULL, 0 );
}




