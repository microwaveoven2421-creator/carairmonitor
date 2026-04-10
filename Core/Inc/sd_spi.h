#ifndef __SD_SPI_H__
#define __SD_SPI_H__

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SD_RES_OK = 0,
    SD_RES_ERROR,
    SD_RES_WRPRT,
    SD_RES_NOTRDY,
    SD_RES_PARERR,
    SD_RES_TIMEOUT
} SD_Result_t;

SD_Result_t SD_SPI_InitCard(void);
SD_Result_t SD_SPI_ReadBlocks(uint8_t *buff, uint32_t sector, uint32_t count);
SD_Result_t SD_SPI_WriteBlocks(const uint8_t *buff, uint32_t sector, uint32_t count);
SD_Result_t SD_SPI_GetSectorSize(uint16_t *size);
SD_Result_t SD_SPI_GetBlockSize(uint32_t *size);
SD_Result_t SD_SPI_GetSectorCount(uint32_t *count);
uint8_t SD_SPI_IsInitialized(void);

extern volatile uint8_t sd_dbg_step;
extern volatile uint8_t sd_dbg_r1_cmd0;
extern volatile uint8_t sd_dbg_r1_cmd8;
extern volatile uint8_t sd_dbg_r1_acmd41;
extern volatile uint8_t sd_dbg_r1_cmd58;
extern volatile uint8_t sd_dbg_ocr[4];

#ifdef __cplusplus
}
#endif

#endif