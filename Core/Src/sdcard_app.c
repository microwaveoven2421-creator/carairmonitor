#include "sdcard_app.h"
#include "fatfs.h"
#include <string.h>

static FATFS g_fs;
static uint8_t g_sdcard_mounted = 0;

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

    if ((filename == NULL) || (buffer == NULL) || (buffer_size == 0))
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

    fres = f_read(&fil, buffer, buffer_size - 1, &br);
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