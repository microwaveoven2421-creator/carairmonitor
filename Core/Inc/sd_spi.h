#ifndef __SD_SPI_H
#define __SD_SPI_H

#include "main.h"
#include <stdint.h>

typedef enum {
    SD_RES_OK = 0,
    SD_RES_ERROR,
    SD_RES_TIMEOUT,
    SD_RES_NOTRDY
} SD_Result_t;

SD_Result_t SD_SPI_InitCard(void);
SD_Result_t SD_SPI_ReadBlocks(uint8_t *buff, uint32_t sector, uint32_t count);
SD_Result_t SD_SPI_WriteBlocks(const uint8_t *buff, uint32_t sector, uint32_t count);

SD_Result_t SD_SPI_GetSectorCount(uint32_t *count);
SD_Result_t SD_SPI_GetSectorSize(uint16_t *size);
SD_Result_t SD_SPI_GetBlockSize(uint32_t *size);

uint8_t SD_SPI_IsInitialized(void);

#endif /* __SD_SPI_H */