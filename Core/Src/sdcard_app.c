/**
 * @file    sdcard_app.c
 * @brief   基于 FatFs 的 SD 卡应用层。
 * @details 完成挂载、CSV 传感器日志写入，以及文本文件读写等辅助功能。
 * @author  Microwave Oven
 */

#include "sdcard_app.h"
#include "sd_spi.h"
#include "fatfs.h"
#include <stdio.h>
#include <string.h>

static FATFS g_fs;
static uint8_t g_sdcard_mounted = 0;
static uint8_t g_sdcard_ready = 0;

#define SDCARD_LOG_FILE      "sensor_log.csv"
#define SD_LOG_HEADER_LINE   "time_stamp,temperature_c,humidity_rh,pm1_0,pm2_5,pm10_0,alcohol_mgL,co2_ppm,alarm_type\r\n"

static SD_Card_Result_t SD_Card_FromAppResult(SDCARD_APP_Result_t res)
{
    switch (res)
    {
    case SDCARD_APP_OK:
        return SD_CARD_OK;
    case SDCARD_APP_NOT_READY:
        return SD_CARD_NOT_READY;
    case SDCARD_APP_FATFS_ERROR:
        return SD_CARD_FATFS_ERROR;
    case SDCARD_APP_ERROR:
    default:
        return SD_CARD_ERROR;
    }
}

static SDCARD_APP_Result_t SDCARD_APP_MountIfNeeded(void)
{
    FRESULT fres;

    if (g_sdcard_mounted)
    {
        return SDCARD_APP_OK;
    }

    fres = f_mount(&g_fs, "", 1);
    if (fres == FR_OK)
    {
        g_sdcard_mounted = 1;
        return SDCARD_APP_OK;
    }

    return SDCARD_APP_FATFS_ERROR;
}

static SDCARD_APP_Result_t SDCARD_APP_EnsureLogHeader(const char *filename)
{
    FILINFO fno;
    FRESULT fres;

    if (filename == NULL)
    {
        return SDCARD_APP_ERROR;
    }

    if (SDCARD_APP_MountIfNeeded() != SDCARD_APP_OK)
    {
        return SDCARD_APP_NOT_READY;
    }

    fres = f_stat(filename, &fno);
    if (fres == FR_OK)
    {
        if (fno.fsize > 0U)
        {
            return SDCARD_APP_OK;
        }
    }

    return SDCARD_APP_WriteText(filename, SD_LOG_HEADER_LINE);
}

SDCARD_APP_Result_t SDCARD_APP_Init(void)
{
    g_sdcard_mounted = 0;
    return SDCARD_APP_MountIfNeeded();
}

SDCARD_APP_Result_t SDCARD_APP_WriteText(const char *filename, const char *text)
{
    FIL fil;
    FRESULT fres;
    UINT bw;
    UINT len;

    if ((filename == NULL) || (text == NULL))
    {
        return SDCARD_APP_ERROR;
    }

    if (SDCARD_APP_MountIfNeeded() != SDCARD_APP_OK)
    {
        return SDCARD_APP_NOT_READY;
    }

    fres = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
    if (fres != FR_OK)
    {
        return SDCARD_APP_FATFS_ERROR;
    }

    len = (UINT)strlen(text);
    fres = f_write(&fil, text, len, &bw);
    f_close(&fil);

    if ((fres == FR_OK) && (bw == len))
    {
        return SDCARD_APP_OK;
    }

    return SDCARD_APP_FATFS_ERROR;
}

SDCARD_APP_Result_t SDCARD_APP_AppendText(const char *filename, const char *text)
{
    FIL fil;
    FRESULT fres;
    UINT bw;
    UINT len;

    if ((filename == NULL) || (text == NULL))
    {
        return SDCARD_APP_ERROR;
    }

    if (SDCARD_APP_MountIfNeeded() != SDCARD_APP_OK)
    {
        return SDCARD_APP_NOT_READY;
    }

    fres = f_open(&fil, filename, FA_OPEN_ALWAYS | FA_WRITE);
    if (fres != FR_OK)
    {
        return SDCARD_APP_FATFS_ERROR;
    }

    fres = f_lseek(&fil, f_size(&fil));
    if (fres != FR_OK)
    {
        f_close(&fil);
        return SDCARD_APP_FATFS_ERROR;
    }

    len = (UINT)strlen(text);
    fres = f_write(&fil, text, len, &bw);
    f_close(&fil);

    if ((fres == FR_OK) && (bw == len))
    {
        return SDCARD_APP_OK;
    }

    return SDCARD_APP_FATFS_ERROR;
}

SDCARD_APP_Result_t SDCARD_APP_ReadText(const char *filename, char *buffer, uint32_t buffer_size)
{
    FIL fil;
    FRESULT fres;
    UINT br;

    if ((filename == NULL) || (buffer == NULL) || (buffer_size == 0U))
    {
        return SDCARD_APP_ERROR;
    }

    if (SDCARD_APP_MountIfNeeded() != SDCARD_APP_OK)
    {
        return SDCARD_APP_NOT_READY;
    }

    memset(buffer, 0, buffer_size);

    fres = f_open(&fil, filename, FA_READ);
    if (fres != FR_OK)
    {
        return SDCARD_APP_FATFS_ERROR;
    }

    fres = f_read(&fil, buffer, buffer_size - 1U, &br);
    f_close(&fil);

    if (fres == FR_OK)
    {
        buffer[br] = '\0';
        return SDCARD_APP_OK;
    }

    return SDCARD_APP_FATFS_ERROR;
}

SDCARD_APP_Result_t SDCARD_APP_FileExists(const char *filename)
{
    FILINFO fno;
    FRESULT fres;

    if (filename == NULL)
    {
        return SDCARD_APP_ERROR;
    }

    if (SDCARD_APP_MountIfNeeded() != SDCARD_APP_OK)
    {
        return SDCARD_APP_NOT_READY;
    }

    fres = f_stat(filename, &fno);
    if (fres == FR_OK)
    {
        return SDCARD_APP_OK;
    }

    return SDCARD_APP_ERROR;
}

SDCARD_APP_Result_t SDCARD_APP_GetFileSize(const char *filename, uint32_t *size)
{
    FILINFO fno;
    FRESULT fres;

    if ((filename == NULL) || (size == NULL))
    {
        return SDCARD_APP_ERROR;
    }

    if (SDCARD_APP_MountIfNeeded() != SDCARD_APP_OK)
    {
        return SDCARD_APP_NOT_READY;
    }

    fres = f_stat(filename, &fno);
    if (fres == FR_OK)
    {
        *size = fno.fsize;
        return SDCARD_APP_OK;
    }

    return SDCARD_APP_FATFS_ERROR;
}

SD_Card_Result_t SD_Card_Init(void)
{
    if (SD_SPI_InitCard() != SD_RES_OK)
    {
        g_sdcard_ready = 0;
        g_sdcard_mounted = 0;
        return SD_CARD_ERROR;
    }

    if (SDCARD_APP_Init() != SDCARD_APP_OK)
    {
        g_sdcard_ready = 0;
        return SD_CARD_FATFS_ERROR;
    }

    g_sdcard_ready = 1;
    return SD_CARD_OK;
}

uint8_t SD_Card_IsReady(void)
{
    return (uint8_t)(g_sdcard_ready && SD_SPI_IsInitialized());
}

SD_Card_Result_t SD_Card_WriteTextFile(const char *filename, const char *text)
{
    if (!SD_Card_IsReady())
    {
        return SD_CARD_NOT_READY;
    }

    return SD_Card_FromAppResult(SDCARD_APP_WriteText(filename, text));
}

SD_Card_Result_t SD_Card_AppendTextFile(const char *filename, const char *text)
{
    if (!SD_Card_IsReady())
    {
        return SD_CARD_NOT_READY;
    }

    return SD_Card_FromAppResult(SDCARD_APP_AppendText(filename, text));
}

SD_Card_Result_t SD_Card_LogSensorData(const SD_SensorRecord_t *record)
{
    char line[160];
    int len;
    SDCARD_APP_Result_t app_res;

    if (record == NULL)
    {
        return SD_CARD_ERROR;
    }

    if (!SD_Card_IsReady())
    {
        return SD_CARD_NOT_READY;
    }

    app_res = SDCARD_APP_EnsureLogHeader(SDCARD_LOG_FILE);
    if (app_res != SDCARD_APP_OK)
    {
        return SD_Card_FromAppResult(app_res);
    }

    len = snprintf(line,
                   sizeof(line),
                   "%s,%.2f,%.2f,%u,%u,%u,%.2f,%u,%u\r\n",
                   record->time_stamp,
                   record->temperature,
                   record->humidity,
                   (unsigned int)record->pm1_0,
                   (unsigned int)record->pm2_5,
                   (unsigned int)record->pm10_0,
                   record->alcohol_mgL,
                   (unsigned int)record->co2,
                   (unsigned int)record->alarm_type);

    if ((len <= 0) || ((size_t)len >= sizeof(line)))
    {
        return SD_CARD_ERROR;
    }

    return SD_Card_FromAppResult(SDCARD_APP_AppendText(SDCARD_LOG_FILE, line));
}

SD_Card_Result_t SD_Card_ReadSensorLog(SD_SensorRecord_t *records, uint32_t max_records, uint32_t *read_count)
{
    FIL fil;
    FRESULT fres;
    char line[256];
    uint32_t count = 0;
    int parsed;

    if ((records == NULL) || (max_records == 0) || (read_count == NULL))
    {
        return SD_CARD_ERROR;
    }

    if (!SD_Card_IsReady())
    {
        return SD_CARD_NOT_READY;
    }

    fres = f_open(&fil, SDCARD_LOG_FILE, FA_READ);
    if (fres != FR_OK)
    {
        return SD_CARD_FATFS_ERROR;
    }

    // Skip header line
    if (f_gets(line, sizeof(line), &fil) == NULL)
    {
        f_close(&fil);
        return SD_CARD_ERROR;
    }

    while ((count < max_records) && (f_gets(line, sizeof(line), &fil) != NULL))
    {
        // Parse CSV line: time_stamp,temperature,humidity,pm1_0,pm2_5,pm10_0,alcohol,co2,alarm_type
        parsed = sscanf(line, "%31[^,],%f,%f,%hu,%hu,%hu,%f,%hu,%hhu",
                        records[count].time_stamp,
                        &records[count].temperature,
                        &records[count].humidity,
                        &records[count].pm1_0,
                        &records[count].pm2_5,
                        &records[count].pm10_0,
                        &records[count].alcohol_mgL,
                        &records[count].co2,
                        &records[count].alarm_type);

        if (parsed == 9)
        {
            count++;
        }
        else
        {
            // Skip invalid lines
        }
    }

    f_close(&fil);
    *read_count = count;
    return SD_CARD_OK;
}

SD_Card_Result_t SD_Card_ReadTextFile(const char *filename, char *buffer, uint32_t buffer_size)
{
    if (!SD_Card_IsReady())
    {
        return SD_CARD_NOT_READY;
    }

    return SD_Card_FromAppResult(SDCARD_APP_ReadText(filename, buffer, buffer_size));
}

SD_Card_Result_t SD_Card_GetFileSize(const char *filename, uint32_t *size)
{
    if (!SD_Card_IsReady())
    {
        return SD_CARD_NOT_READY;
    }

    return SD_Card_FromAppResult(SDCARD_APP_GetFileSize(filename, size));
}

SD_Card_Result_t SD_Card_DeleteFile(const char *filename)
{
    FRESULT fres;

    if (filename == NULL)
    {
        return SD_CARD_ERROR;
    }

    if (!SD_Card_IsReady())
    {
        return SD_CARD_NOT_READY;
    }

    fres = f_unlink(filename);
    if (fres == FR_OK)
    {
        return SD_CARD_OK;
    }

    return SD_CARD_FATFS_ERROR;
}
// #include "sdcard_app.h"
// #include "fatfs.h"
// #include <string.h>

// static FATFS g_fs;
// static uint8_t g_sdcard_mounted = 0;

// static SDCARD_APP_Result_t SDCARD_APP_MountIfNeeded(void)
// {
//     FRESULT fres;

//     if (g_sdcard_mounted)
//     {
//         return SDCARD_APP_OK;
//     }

//     fres = f_mount(&g_fs, "", 1);
//     if (fres == FR_OK)
//     {
//         g_sdcard_mounted = 1;
//         return SDCARD_APP_OK;
//     }

//     return SDCARD_APP_FATFS_ERROR;
// }

// SDCARD_APP_Result_t SDCARD_APP_Init(void)
// {
//     g_sdcard_mounted = 0;
//     return SDCARD_APP_MountIfNeeded();
// }

// SDCARD_APP_Result_t SDCARD_APP_WriteText(const char *filename, const char *text)
// {
//     FIL fil;
//     FRESULT fres;
//     UINT bw;
//     UINT len;

//     if ((filename == NULL) || (text == NULL))
//     {
//         return SDCARD_APP_ERROR;
//     }

//     if (SDCARD_APP_MountIfNeeded() != SDCARD_APP_OK)
//     {
//         return SDCARD_APP_NOT_READY;
//     }

//     fres = f_open(&fil, filename, FA_CREATE_ALWAYS | FA_WRITE);
//     if (fres != FR_OK)
//     {
//         return SDCARD_APP_FATFS_ERROR;
//     }

//     len = (UINT)strlen(text);
//     fres = f_write(&fil, text, len, &bw);
//     f_close(&fil);

//     if ((fres == FR_OK) && (bw == len))
//     {
//         return SDCARD_APP_OK;
//     }

//     return SDCARD_APP_FATFS_ERROR;
// }

// SDCARD_APP_Result_t SDCARD_APP_AppendText(const char *filename, const char *text)
// {
//     FIL fil;
//     FRESULT fres;
//     UINT bw;
//     UINT len;

//     if ((filename == NULL) || (text == NULL))
//     {
//         return SDCARD_APP_ERROR;
//     }

//     if (SDCARD_APP_MountIfNeeded() != SDCARD_APP_OK)
//     {
//         return SDCARD_APP_NOT_READY;
//     }

//     fres = f_open(&fil, filename, FA_OPEN_ALWAYS | FA_WRITE);
//     if (fres != FR_OK)
//     {
//         return SDCARD_APP_FATFS_ERROR;
//     }

//     fres = f_lseek(&fil, f_size(&fil));
//     if (fres != FR_OK)
//     {
//         f_close(&fil);
//         return SDCARD_APP_FATFS_ERROR;
//     }

//     len = (UINT)strlen(text);
//     fres = f_write(&fil, text, len, &bw);
//     f_close(&fil);

//     if ((fres == FR_OK) && (bw == len))
//     {
//         return SDCARD_APP_OK;
//     }

//     return SDCARD_APP_FATFS_ERROR;
// }

// SDCARD_APP_Result_t SDCARD_APP_ReadText(const char *filename, char *buffer, uint32_t buffer_size)
// {
//     FIL fil;
//     FRESULT fres;
//     UINT br;

//     if ((filename == NULL) || (buffer == NULL) || (buffer_size == 0))
//     {
//         return SDCARD_APP_ERROR;
//     }

//     if (SDCARD_APP_MountIfNeeded() != SDCARD_APP_OK)
//     {
//         return SDCARD_APP_NOT_READY;
//     }

//     memset(buffer, 0, buffer_size);

//     fres = f_open(&fil, filename, FA_READ);
//     if (fres != FR_OK)
//     {
//         return SDCARD_APP_FATFS_ERROR;
//     }

//     fres = f_read(&fil, buffer, buffer_size - 1, &br);
//     f_close(&fil);

//     if (fres == FR_OK)
//     {
//         buffer[br] = '\0';
//         return SDCARD_APP_OK;
//     }

//     return SDCARD_APP_FATFS_ERROR;
// }

// SDCARD_APP_Result_t SDCARD_APP_FileExists(const char *filename)
// {
//     FILINFO fno;
//     FRESULT fres;

//     if (filename == NULL)
//     {
//         return SDCARD_APP_ERROR;
//     }

//     if (SDCARD_APP_MountIfNeeded() != SDCARD_APP_OK)
//     {
//         return SDCARD_APP_NOT_READY;
//     }

//     fres = f_stat(filename, &fno);
//     if (fres == FR_OK)
//     {
//         return SDCARD_APP_OK;
//     }

//     return SDCARD_APP_ERROR;
// }

// SDCARD_APP_Result_t SDCARD_APP_GetFileSize(const char *filename, uint32_t *size)
// {
//     FILINFO fno;
//     FRESULT fres;

//     if ((filename == NULL) || (size == NULL))
//     {
//         return SDCARD_APP_ERROR;
//     }

//     if (SDCARD_APP_MountIfNeeded() != SDCARD_APP_OK)
//     {
//         return SDCARD_APP_NOT_READY;
//     }

//     fres = f_stat(filename, &fno);
//     if (fres == FR_OK)
//     {
//         *size = fno.fsize;
//         return SDCARD_APP_OK;
//     }

//     return SDCARD_APP_FATFS_ERROR;
// }
