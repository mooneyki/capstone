#include <stdio.h>
#include "esp_types.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "soc/timer_group_struct.h"
#include "driver/periph_ctrl.h"
#include "driver/timer.h"

#include "nubaja_gpio.h"
#include "nubaja_i2c.h"
#include "nubaja_ad7998.h"
#include "nubaja_sd.h"
#include "nubaja_pid.h"
#include "nubaja_pwm.h"

#define DAQ_TIMER_GROUP       TIMER_GROUP_0  // group of daq timer
#define DAQ_TIMER_IDX         0              // index of daq timer
#define DAQ_TIMER_HZ          1000           // frequency of the daq timer in Hz
#define DAQ_TIMER_DIVIDER     100

xQueueHandle daq_timer_queue; // queue to time the daq task
xQueueHandle logging_queue_1, logging_queue_2, current_dp_queue; // queues to store data points

pid_t throttle_pid;
pid_t brake_current_pid;

// interrupt for daq_task timer
void IRAM_ATTR daq_timer_isr(void *para)
{
  // retrieve the interrupt status and the counter value from the timer
  uint32_t intr_status = TIMERG0.int_st_timers.val;
  TIMERG0.hw_timer[DAQ_TIMER_IDX].update = 1;

  // clear the interrupt
  if (intr_status & BIT(DAQ_TIMER_IDX))
  {
    TIMERG0.int_clr_timers.t0 = 1;
  }

  // enable the alarm again, so it is triggered the next time
  TIMERG0.hw_timer[DAQ_TIMER_IDX].config.alarm_en = TIMER_ALARM_EN;

  // send the counter value to the queue to trigger the daq task
  xQueueOverwriteFromISR(daq_timer_queue, &intr_status, NULL);
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
  timer_set_counter_value(DAQ_TIMER_GROUP, DAQ_TIMER_IDX, 0x00000000ULL);

  // configure the alarm value and the interrupt on alarm
  timer_set_alarm_value(DAQ_TIMER_GROUP, DAQ_TIMER_IDX, TIMER_BASE_CLK / DAQ_TIMER_DIVIDER / DAQ_TIMER_HZ);
  timer_enable_intr(DAQ_TIMER_GROUP, DAQ_TIMER_IDX);
  timer_isr_register(DAQ_TIMER_GROUP, DAQ_TIMER_IDX, daq_timer_isr, NULL, ESP_INTR_FLAG_IRAM, NULL);

  timer_start(DAQ_TIMER_GROUP, DAQ_TIMER_IDX);
}

// task to run the main daq system based on a timer
static void daq_task(void *arg)
{
  // initial config

  // init ADC
  i2c_master_config(PORT_0, FAST_MODE_PLUS, I2C_MASTER_0_SDA_IO,I2C_MASTER_0_SCL_IO);
  ad7998_init( PORT_0, ADC_SLAVE_ADDR, CHANNEL_SELECTION_0 ); //init adc

  // init sd
  init_sd();
  xQueueHandle current_logging_queue = logging_queue_1;

  //init GPIOs
  configure_gpio();

  //init PIDs
  init_pid ( throttle_pid, 0, 0, 0, 0, 0 ); 
  init_pid ( brake_current_pid, 0, 0, 0, 0, 0 );

  // button vars
  uint8_t buttons, logging_enabled, data_to_log;

  uint32_t intr_status;
  while (1)
  {
    // wait for timer alarm
    xQueueReceive(daq_timer_queue, &intr_status, portMAX_DELAY);

    // get button flags
    logging_enabled = buttons & ENABLE_LOGGING_BIT;

    // flasher if logging
    if (logging_enabled)
      flasher_on();
    else
      flasher_off();

    // create data struct and populate with this cycle's data
    data_point dp =
    {
      dp->prim_rpm = 0,    dp->sec_rpm = 0,        dp->torque = 0,
      dp->temp3 = 0,       dp->belt_temp = 0,      dp->temp2 = 0,
      dp->i_brake = 0,     dp->temp1 = 0,          dp->load_cell = 0, 
      dp->tps = 0
    };

    // imu
    imu_read_gyro_xl(&imu, &(dp.gyro_x), &(dp.gyro_y), &(dp.gyro_z),
                           &(dp.xl_x), &(dp.xl_y), &(dp.xl_z));

    // rpm measurements
    xQueuePeek(primary_rpm_queue, &(dp.prim_rpm), 0);
    xQueuePeek(secondary_rpm_queue, &(dp.sec_rpm), 0);

    // TODO: temp
    dp.temp = 0;

    // push struct to current dp queue for display
    xQueueOverwrite(current_dp_queue, &dp);

    // push struct to logging queue
    // if the queue is full, switch queues and send the full for writing to SD
    if ((logging_enabled && xQueueSend(current_logging_queue, &dp, 0) == errQUEUE_FULL)
        || (!logging_enabled && data_to_log))
    {
      printf("daq_task -- queue full, writing and switiching...\n");
      if (current_logging_queue == logging_queue_1)
      {
        current_logging_queue = logging_queue_2;
        xTaskCreatePinnedToCore(write_logging_queue_to_sd,
                "write_lq_1_sd", 2048, (void *) logging_queue_1,
                (configMAX_PRIORITIES-1), NULL, 1);
      }
      else
      {
        current_logging_queue = logging_queue_1;
        xTaskCreatePinnedToCore(write_logging_queue_to_sd,
                "write_lq_2_sd", 2048, (void *) logging_queue_2,
                (configMAX_PRIORITIES-1), NULL, 1);
      }

      // reset, though queue should be empty after writing
      // queue won't be empty if a writing task overlap occurred
      xQueueReset(current_logging_queue);

      // if writing data after logging disabled, clear DATA_TO_LOG
      // else add dp to new current buffer
      if (!logging_enabled)
        xEventGroupClearBits(button_eg, DATA_TO_LOG_BIT);
      else
        xQueueSend(current_logging_queue, &dp, 0);
    }
  }
  // per FreeRTOS, tasks MUST be deleted before breaking out of its implementing funciton
  vTaskDelete(NULL);
}

// initialize the daq timer and start the daq task
void app_main()
{
  daq_timer_queue = xQueueCreate(1, sizeof(uint32_t));

  logging_queue_1 = xQueueCreate(LOGGING_QUEUE_SIZE, sizeof(data_point));
  logging_queue_2 = xQueueCreate(LOGGING_QUEUE_SIZE, sizeof(data_point));

  // setup current data point queue
  current_dp_queue = xQueueCreate(1, sizeof(data_point));
  data_point dp =
  {
    dp->prim_rpm = 0,    dp->sec_rpm = 0,        dp->torque = 0,
    dp->temp3 = 0,       dp->belt_temp = 0,      dp->temp2 = 0,
    dp->i_brake = 0,     dp->temp1 = 0,          dp->load_cell = 0, 
    dp->tps = 0
  };
  xQueueOverwrite(current_dp_queue, &dp);

  // start daq timer and tasks
  daq_timer_init();

  xTaskCreatePinnedToCore(daq_task, "daq_task", 2048,
                          NULL, (configMAX_PRIORITIES-1), NULL, 0);
}
