/**
 * @file    sdcard_app.h
 * @brief   SD 卡日志记录和文件工具接口。
 * @details 定义传感器日志记录结构体，以及高层 SD 卡读写辅助函数。
 * @author  Microwave Oven
 */

#ifndef __SDCARD_APP_H__
#define __SDCARD_APP_H__

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SDCARD_APP_OK = 0,
    SDCARD_APP_ERROR,
    SDCARD_APP_NOT_READY,
    SDCARD_APP_FATFS_ERROR
} SDCARD_APP_Result_t;

typedef enum {
    SD_CARD_OK = 0,
    SD_CARD_ERROR,
    SD_CARD_NOT_READY,
    SD_CARD_FATFS_ERROR
} SD_Card_Result_t;

typedef struct {
    char     time_stamp[32];
    float    temperature;
    float    humidity;
    uint16_t pm1_0;
    uint16_t pm2_5;
    uint16_t pm10_0;
    float    alcohol_mgL;
    uint16_t co2;
    uint8_t  alarm_type;
} SD_SensorRecord_t;

SDCARD_APP_Result_t SDCARD_APP_Init(void);
SDCARD_APP_Result_t SDCARD_APP_WriteText(const char *filename, const char *text);
SDCARD_APP_Result_t SDCARD_APP_AppendText(const char *filename, const char *text);
SDCARD_APP_Result_t SDCARD_APP_ReadText(const char *filename, char *buffer, uint32_t buffer_size);
SDCARD_APP_Result_t SDCARD_APP_FileExists(const char *filename);
SDCARD_APP_Result_t SDCARD_APP_GetFileSize(const char *filename, uint32_t *size);

SD_Card_Result_t SD_Card_Init(void);
uint8_t SD_Card_IsReady(void);
SD_Card_Result_t SD_Card_LogSensorData(const SD_SensorRecord_t *record);
SD_Card_Result_t SD_Card_WriteTextFile(const char *filename, const char *text);
SD_Card_Result_t SD_Card_AppendTextFile(const char *filename, const char *text);
SD_Card_Result_t SD_Card_ReadSensorLog(SD_SensorRecord_t *records, uint32_t max_records, uint32_t *read_count);
SD_Card_Result_t SD_Card_ReadTextFile(const char *filename, char *buffer, uint32_t buffer_size);
SD_Card_Result_t SD_Card_GetFileSize(const char *filename, uint32_t *size);
SD_Card_Result_t SD_Card_DeleteFile(const char *filename);

#ifdef __cplusplus
}
#endif

#endif
