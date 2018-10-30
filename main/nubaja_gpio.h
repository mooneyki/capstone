#ifndef NUBAJA_GPIO_H_
#define NUBAJA_GPIO_H_

#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "driver/gpio.h"

#define PRIMARY_GPIO          26             // engine rpm measurement
#define SECONDARY_GPIO        27             // CVT secondary rpm measurement
#define SOLENOID_GPIO         35             //e-brake release solenoid
#define KILL_GPIO             33             //kill switch relay control
#define FLASHER_GPIO          32             // flashing indicator relay control
#define GPIO_INPUT_PIN_SEL    ((1ULL<<PRIMARY_GPIO) | (1ULL<<SECONDARY_GPIO))

#define RPM_TIMER_GROUP     TIMER_GROUP_1  // group of speed timer
#define RPM_TIMER_IDX       0              // index of speed timer
#define RPM_TIMER_DIVIDER   100            // speed timer prescale divider

#define MAX_PRIMARY_RPM       4500           // cut off wacky high errors
#define MAX_SECONDARY_RPM     4500            // cut off wacky high errors

xQueueHandle primary_rpm_queue;                    // queue for engine rpm values
xQueueHandle secondary_rpm_queue;                    // queue for wheel speed values

double last_prim_rpm_time = 0;
double last_sec_rpm_time = 0;

void flasher_on()
{
  gpio_set_level(FLASHER_GPIO, 1);
}

void flasher_off()
{
  gpio_set_level(FLASHER_GPIO, 0);
}

void ebrake_set() 
{
  gpio_set_level(SOLENOID_GPIO, 0);
}

void ebrake_release() 
{
  gpio_set_level(SOLENOID_GPIO, 1);  
}

void engine_off() 
{
  gpio_set_level(KILL_GPIO, 1);   
}

void engine_on() 
{
  gpio_set_level(KILL_GPIO, 0);   
}

static void secondary_isr_handler(void *arg)
{
  double time;
  timer_get_counter_time_sec(RPM_TIMER_GROUP, RPM_TIMER_IDX, &time);
  uint16_t rpm = 60.0 / (time - last_sec_rpm_time);
  
  if (rpm <= MAX_SECONDARY_RPM)
  {
    xQueueOverwriteFromISR(secondary_rpm_queue, &rpm, NULL);
  }

  last_sec_rpm_time = time;
}

static void primary_isr_handler(void *arg)
{
  double time;
  timer_get_counter_time_sec(RPM_TIMER_GROUP, RPM_TIMER_IDX, &time);
  uint16_t rpm = 60.0 / (time - last_prim_rpm_time);
  
  if (rpm <= MAX_PRIMARY_RPM)
  {
    xQueueOverwriteFromISR(primary_rpm_queue, &rpm, NULL);
  }

  last_prim_rpm_time = time;
}

static void speed_timer_init()
{
  // select and initialize basic parameters of the timer
  timer_config_t config;
  config.divider = RPM_TIMER_DIVIDER;
  config.counter_dir = TIMER_COUNT_UP;
  config.counter_en = TIMER_PAUSE;
  config.alarm_en = TIMER_ALARM_DIS;
  config.intr_type = TIMER_INTR_LEVEL;
  config.auto_reload = TIMER_AUTORELOAD_DIS;
  timer_init(SPEED_TIMER_GROUP, SPEED_TIMER_IDX, &config);

  // timer's counter will initially start from value below
  timer_set_counter_value(RPM_TIMER_GROUP, RPM_TIMER_IDX, 0x00000000ULL);

  timer_start(RPM_TIMER_GROUP, RPM_TIMER_IDX);
}

// configure gpio pins for input and ISRs, and the flasher pin for output
void configure_gpio()
{
  // setup timer and queues for speeds
  speed_timer_init();
  primary_rpm_queue = xQueueCreate(1, sizeof(uint16_t));
  secondary_rpm_queue = xQueueCreate(1, sizeof(uint16_t));

  // config rising-edge interrupt GPIO pins (rpm, mph, and logging)
  gpio_config_t io_conf;
  io_conf.intr_type = GPIO_PIN_INTR_POSEDGE;  // interrupt of rising edge
  io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;  // bit mask of the pins
  io_conf.mode = GPIO_MODE_INPUT;  // set as input mode
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  gpio_config(&io_conf);

  // ISRs
  gpio_install_isr_service(0); // install gpio isr service
  gpio_isr_handler_add(PRIMARY_GPIO, primary_isr_handler, NULL); 
  gpio_isr_handler_add(SECONDARY_GPIO, secondary_isr_handler, NULL); 

  // GPIOs
  gpio_set_direction(FLASHER_GPIO, GPIO_MODE_OUTPUT);
  gpio_set_direction(KILL_GPIO, GPIO_MODE_OUTPUT);
  gpio_set_direction(SOLENOID_GPIO, GPIO_MODE_OUTPUT);

  flasher_off();
  engine_off();
  ebrake_set();
}

#endif // NUBAJA_GPIO_H_
