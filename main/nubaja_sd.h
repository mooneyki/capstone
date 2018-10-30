#ifndef NUBAJA_SD_H_
#define NUBAJA_SD_H_

#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

#define SD_MISO 19
#define SD_MOSI 18
#define SD_CLK  14
#define SD_CS   15

#define LOGGING_QUEUE_SIZE  1000   // data logging queue size

#define WRITING_DATA_BIT (1 << 0)
EventGroupHandle_t writing_eg;  // event group to signify writing data

typedef struct
{
  uint16_t prim_rpm, sec_rpm;
  uint16_t torque, temp3, belt_temp, temp2, temp1, load_cell, tps, i_brake;
  float i_sp, tps_sp; 
} data_point;

void print_data_point(data_point *dp)
{
  printf("primary rpm:\t%" PRIu16       "\tsecondary rpm:\t%" PRIu16    "\ttorque:\t%" PRIu16 "\n"
         "temp3:\t%" PRIu16             "\tbelt temp:\t%" PRIu16        "\ttemp2:\t%" PRIu16 "\n"
         "brake current:\t%" PRIu16     "\ttemp1:\t%" PRIu16            "\tload cell:\t%" PRIu16 "\n",
         "throttle position:\t%" PRIu16 "\ti_sp:\t%" %5.1f              "\ttps_sp:\t%" %5.1f "\n",
         dp->prim_rpm,                  dp->sec_rpm,                    dp->torque,
         dp->temp3,                     dp->belt_temp,                  dp->temp2,
         dp->i_brake,                   dp->temp1,                      dp->load_cell, 
         dp->tps                        dp->i_sp                        dp->tps_sp); //NEED TO FIX TYPES HERE
}

static void write_logging_queue_to_sd(void *arg)
{
  if (xEventGroupGetBits(writing_eg) & WRITING_DATA_BIT)
  {
    printf("write_logging_queue_to_sd -- task overlap, skipping queue\n");
    vTaskDelete(NULL);
  }
  else
  {
    xEventGroupSetBits(writing_eg, WRITING_DATA_BIT);
  }

  xQueueHandle lq = (xQueueHandle) arg;
  data_point dp;
  // num fields * num chars for each value + comma / return
  // int16_t can be -35,xxx, so max 6 chars per val
  int line_size = 9 * 7;
  // num lines * line size + char for null term
  int buff_size = LOGGING_QUEUE_SIZE * line_size + 1;
  char* buff = (char*) malloc(buff_size * sizeof(char));

  int i = 0;
  xQueueReceive(lq, &dp, 0);
  while (xQueueReceive(lq, &dp, 0) != pdFALSE)
  {
    snprintf(buff + (i * line_size), buff_size - (i * line_size),
             "%6" PRIu16 ", %6" PRIu16 ",   %6" PRIu16 ","
             "%6" PRIu16 ", %6" PRIu16 ",   %6" PRIu16 ","
             "%6" PRIu16 ", %6" PRIu16 ",   %6" PRIu16 ","
             "%6" PRIu16 ", %6" %5.1f ",    %6" %5.1f "\n", 
              dp->prim_rpm, dp->sec_rpm,    dp->torque,
              dp->temp3,    dp->belt_temp,  dp->temp2,
              dp->i_brake,  dp->temp1,      dp->load_cell, 
              dp->tps       dp->i_sp        dp->tps_sp); //NEED TO FIX THE TYPE HERE
    ++i;
  }

  FILE *fp;
  fp = fopen("/sdcard/esp_dyno_data.csv", "a");
  if (fp == NULL)
  {
    printf("write_logging_queue_to_sd -- failed to create file\n");
    vTaskDelete(NULL);
  }
  fprintf(fp, "%s", buff);
  fclose(fp);

  printf("write_logging_queue_to_sd -- writing done\n");
  free(buff);
  xEventGroupClearBits(writing_eg, WRITING_DATA_BIT);

  // per FreeRTOS, tasks MUST be deleted before breaking out of its implementing funciton
  vTaskDelete(NULL);
}

void init_sd()
{
  printf("init_sd -- configuring SD storage\n");

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();

  sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
  slot_config.gpio_miso = SD_MISO;
  slot_config.gpio_mosi = SD_MOSI;
  slot_config.gpio_sck  = SD_CLK;
  slot_config.gpio_cs   = SD_CS;

  esp_vfs_fat_sdmmc_mount_config_t mount_config =
  {
    .format_if_mount_failed = false,
    .max_files = 5
  };

  sdmmc_card_t* card;
  if (esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card)
      != ESP_OK)
  {
    printf("init_sd -- failed to mount SD card\n");
  }

  // setup button event group
  writing_eg = xEventGroupCreate();
  xEventGroupClearBits(writing_eg, WRITING_DATA_BIT);
}

#endif // NUBAJA_SD_H_
