#ifndef __SDCARD_APP_H
#define __SDCARD_APP_H

#include "main.h"
#include "ff.h"
#include <stdint.h>

typedef enum
{
    SDCARD_APP_OK = 0,
    SDCARD_APP_ERROR,
    SDCARD_APP_NOT_READY,
    SDCARD_APP_FATFS_ERROR
} SDCARD_APP_Result_t;

SDCARD_APP_Result_t SDCARD_APP_Init(void);

SDCARD_APP_Result_t SDCARD_APP_WriteText(const char *filename, const char *text);
SDCARD_APP_Result_t SDCARD_APP_AppendText(const char *filename, const char *text);
SDCARD_APP_Result_t SDCARD_APP_ReadText(const char *filename, char *buffer, uint32_t buffer_size);

SDCARD_APP_Result_t SDCARD_APP_FileExists(const char *filename);
SDCARD_APP_Result_t SDCARD_APP_GetFileSize(const char *filename, uint32_t *size);

#endif /* __SDCARD_APP_H */