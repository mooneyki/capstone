#ifndef NUBAJA_SD_H_
#define NUBAJA_SD_H_

#define __STDC_FORMAT_MACROS

#include <inttypes.h>
#include "esp_vfs_fat.h"
#include "driver/sdmmc_host.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"
#include "freertos/semphr.h"

#define SD_MISO 19
#define SD_MOSI 18
#define SD_CLK  14
#define SD_CS   15

#define LOGGING_QUEUE_SIZE  1000   // data logging queue size
SemaphoreHandle_t write_lock = NULL;
int file_num = 0;
char filename[20] = "/sdcard/data_x.csv";

typedef struct
{
  uint16_t prim_rpm, sec_rpm;
  uint16_t torque, temp3, belt_temp, temp2, temp1, load_cell, tps, i_brake;
  float i_sp, tps_sp; 
} data_point;

void print_data_point(data_point *dp)
{
  printf("prrpm:\t%" PRIu16             "\tserpm:\t%" PRIu16            "\ttrque:\t%" PRIu16 "\n"
         "temp3:\t%" PRIu16             "\tbtemp:\t%" PRIu16            "\ttemp2:\t%" PRIu16 "\n"
         "braki:\t%" PRIu16             "\ttemp1:\t%" PRIu16            "\tloadc:\t%" PRIu16 "\n"
         "t_p_s:\t%" PRIu16             "\tbi_sp: %6.2f"                "\ttpssp: %6.2f"  "\n",
         dp->prim_rpm,                  dp->sec_rpm,                    dp->torque,
         dp->temp3,                     dp->belt_temp,                  dp->temp2,
         dp->i_brake,                   dp->temp1,                      dp->load_cell, 
         dp->tps,                       dp->i_sp,                       dp->tps_sp); 
}

static void write_logging_queue_to_sd(void *arg)
{
  if( xSemaphoreTake( write_lock, ( TickType_t ) 1 ) == pdFALSE )
  {
    printf("write_logging_queue_to_sd -- task overlap, skipping queue\n");
    vTaskDelete(NULL);
  }

  xQueueHandle lq = (xQueueHandle) arg;
  data_point dp;
  // num fields * num chars for each value + comma / return
  // int16_t can be -35,xxx, so max 6 chars per val
  int line_size = (13 * 7) + 9; //THIS IS CRITICAL - DO NOT CHANGE
  // num lines * line size + char for null term
  int buff_size = LOGGING_QUEUE_SIZE * line_size + 1;
  char* buff = (char*) malloc(buff_size * sizeof(char));

  int i = 0;
  while (xQueueReceive(lq, &dp, 0) != pdFALSE)
  {
    snprintf(buff + (i * line_size), buff_size - (i * line_size),
             "%6" PRIu16 ", %6" PRIu16 ",   %6" PRIu16 ","
             "%6" PRIu16 ", %6" PRIu16 ",   %6" PRIu16 ","
             "%6" PRIu16 ", %6" PRIu16 ",   %6" PRIu16 ","
             "%6" PRIu16 ", %6.2f"       ",   %6.2f" "\n", 
              dp.prim_rpm,  dp.sec_rpm,     dp.torque,
              dp.temp3,     dp.belt_temp,   dp.temp2,
              dp.i_brake,   dp.temp1,       dp.load_cell, 
              dp.tps,       dp.i_sp,        dp.tps_sp); 
    ++i;
  }
  // for ( i = 0; i<5 ;i++ ) {
  //   xQueueReceive(lq, &dp, 0);
  //   snprintf(buff + (i * line_size), buff_size - (i * line_size),
  //            "%6" PRIu16 ", %6" PRIu16 ",   %6" PRIu16 ","
  //            "%6" PRIu16 ", %6" PRIu16 ",   %6" PRIu16 ","
  //            "%6" PRIu16 ", %6" PRIu16 ",   %6" PRIu16 ","
  //            "%6" PRIu16 ", %6.2f"     ",   %6.2f" "\n", 
  //             dp.prim_rpm,  dp.sec_rpm,     dp.torque,
  //             dp.temp3,     dp.belt_temp,   dp.temp2,
  //             dp.i_brake,   dp.temp1,       dp.load_cell, 
  //             dp.tps,       dp.i_sp,        dp.tps_sp); 
  // }

  FILE *fp;
  fp = fopen( filename, "a" );
  if (fp == NULL)
  {
    printf("write_logging_queue_to_sd -- failed to create file\n");
    vTaskDelete(NULL);
  }
  fprintf(fp, "%s", buff);
  printf("%s",buff);
  fclose(fp);

  printf("write_logging_queue_to_sd -- writing done\n");
  free(buff);

  // per FreeRTOS, tasks MUST be deleted before breaking out of its implementing funciton
  //also release mutex
  xSemaphoreGive ( write_lock );
  vTaskDelete(NULL);
}

void init_sd()
{
  // printf("init_sd -- configuring SD storage\n");

  sdmmc_host_t host = SDSPI_HOST_DEFAULT();

  sdspi_slot_config_t slot_config = SDSPI_SLOT_CONFIG_DEFAULT();
  slot_config.gpio_miso = SD_MISO;
  slot_config.gpio_mosi = SD_MOSI;
  slot_config.gpio_sck  = SD_CLK;
  slot_config.gpio_cs   = SD_CS;

  esp_vfs_fat_sdmmc_mount_config_t mount_config =
  {
    .format_if_mount_failed = true,
    .max_files = 5
  };

  sdmmc_card_t* card;
  esp_err_t ret = esp_vfs_fat_sdmmc_mount("/sdcard", &host, &slot_config, &mount_config, &card);
  if ( ret != ESP_OK )
  {
    if ( ret == ESP_FAIL ) {
      printf("init_sd -- failed to mount filesystem\n");
    }
    else {
      printf("init_sd -- failed to init card\n"); 
    }
  }

  sdmmc_card_print_info(stdout, card);

  //create mutex
  write_lock = xSemaphoreCreateMutex();

  FILE *fp;
  // fp = fopen("/sdcard/data.csv", "a");
  // memset(filename,0,strlen(filename));
  printf("Enter output file num.\n");
  while ( !file_num ) {
    scanf( "%d", &file_num);
  }
  filename[13] = file_num + '0';
  printf("output filename: %s\n",filename);
  fp = fopen( filename, "a");
  if (fp == NULL)
  {
    printf("init_sd -- failed to create file\n");
    fclose(fp);
    return;
  }
  fclose(fp);

  printf("init_sd -- configuring SD success\n");

}

#endif // NUBAJA_SD_H_
