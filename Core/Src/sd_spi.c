#include "sd_spi.h"
#include "spi.h"
#include <string.h>

extern SPI_HandleTypeDef hspi3;

#define SD_CS_GPIO_Port   GPIOA
#define SD_CS_Pin         GPIO_PIN_11

#define SD_CS_HIGH() HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_SET)
#define SD_CS_LOW()  HAL_GPIO_WritePin(SD_CS_GPIO_Port, SD_CS_Pin, GPIO_PIN_RESET)

/* SD commands */
#define CMD0    (0x40 + 0)
#define CMD1    (0x40 + 1)
#define CMD8    (0x40 + 8)
#define CMD9    (0x40 + 9)
#define CMD10   (0x40 + 10)
#define CMD12   (0x40 + 12)
#define CMD16   (0x40 + 16)
#define CMD17   (0x40 + 17)
#define CMD18   (0x40 + 18)
#define CMD23   (0x40 + 23)
#define CMD24   (0x40 + 24)
#define CMD25   (0x40 + 25)
#define CMD55   (0x40 + 55)
#define CMD58   (0x40 + 58)
#define ACMD41  (0x40 + 41)

#define SD_TYPE_UNKNOWN 0
#define SD_TYPE_SDSC    1
#define SD_TYPE_SDHC    2

static uint8_t g_sd_type = SD_TYPE_UNKNOWN;
static uint8_t g_sd_initialized = 0;

volatile uint8_t sd_dbg_step = 0;
volatile uint8_t sd_dbg_r1_cmd0 = 0xFF;
volatile uint8_t sd_dbg_r1_cmd8 = 0xFF;
volatile uint8_t sd_dbg_r1_acmd41 = 0xFF;
volatile uint8_t sd_dbg_r1_cmd58 = 0xFF;
volatile uint8_t sd_dbg_ocr[4] = {0};

/* =========================
   基础 SPI 收发
   ========================= */
static uint8_t SPI_TxRxByte(uint8_t tx)
{
    uint8_t rx = 0xFF;
    if (HAL_SPI_TransmitReceive(&hspi3, &tx, &rx, 1, HAL_MAX_DELAY) != HAL_OK) {
        return 0xFF;
    }
    return rx;
}

static void SD_SendDummyClocks(uint8_t n)
{
    while (n--) {
        SPI_TxRxByte(0xFF);
    }
}

/* =========================
   SPI 速度切换
   初始化低速，完成后高速
   ========================= */
static void SD_SPI_SetSpeedLow(void)
{
    __HAL_SPI_DISABLE(&hspi3);
    hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
    HAL_SPI_Init(&hspi3);
    __HAL_SPI_ENABLE(&hspi3);
}

static void SD_SPI_SetSpeedHigh(void)
{
    __HAL_SPI_DISABLE(&hspi3);
    hspi3.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16;
    HAL_SPI_Init(&hspi3);
    __HAL_SPI_ENABLE(&hspi3);
}

/* =========================
   片选与等待
   ========================= */
static void SD_Deselect(void)
{
    SD_CS_HIGH();
    SPI_TxRxByte(0xFF);
}

static uint8_t SD_WaitReady(uint32_t timeout_ms)
{
    uint8_t res;
    uint32_t tickstart = HAL_GetTick();

    do {
        res = SPI_TxRxByte(0xFF);
        if (res == 0xFF) return 1;
    } while ((HAL_GetTick() - tickstart) < timeout_ms);

    return 0;
}

static uint8_t SD_Select(void)
{
    SD_CS_LOW();
    SPI_TxRxByte(0xFF);

    if (SD_WaitReady(500)) return 1;

    SD_Deselect();
    return 0;
}

/* =========================
   发送命令
   返回 R1 response
   ========================= */
static uint8_t SD_SendCmd(uint8_t cmd, uint32_t arg)
{
    uint8_t buf[6];
    uint8_t crc = 0x01;
    uint8_t res;
    uint8_t n;

    if (cmd == ACMD41) {
        res = SD_SendCmd(CMD55, 0);
        if (res > 1) return res;
    }

    SD_Deselect();
    if (!SD_Select()) return 0xFF;

    buf[0] = cmd;
    buf[1] = (uint8_t)(arg >> 24);
    buf[2] = (uint8_t)(arg >> 16);
    buf[3] = (uint8_t)(arg >> 8);
    buf[4] = (uint8_t)(arg);

    if (cmd == CMD0) crc = 0x95;
    if (cmd == CMD8) crc = 0x87;
    buf[5] = crc;

    for (n = 0; n < 6; n++) {
        SPI_TxRxByte(buf[n]);
    }

    if (cmd == CMD12) {
        SPI_TxRxByte(0xFF);
    }

    n = 10;
    do {
        res = SPI_TxRxByte(0xFF);
    } while ((res & 0x80) && --n);

    return res;
}

/* =========================
   接收 512B 数据块
   token 必须为 0xFE
   ========================= */
static uint8_t SD_RxDataBlock(uint8_t *buff, uint16_t len)
{
    uint8_t token;
    uint32_t tickstart = HAL_GetTick();

    do {
        token = SPI_TxRxByte(0xFF);
        if (token == 0xFE) break;
    } while ((HAL_GetTick() - tickstart) < 200);

    if (token != 0xFE) return 0;

    while (len--) {
        *buff++ = SPI_TxRxByte(0xFF);
    }

    /* discard CRC */
    SPI_TxRxByte(0xFF);
    SPI_TxRxByte(0xFF);

    return 1;
}

/* =========================
   发送 512B 数据块
   token: 0xFE 单块写开始
   ========================= */
static uint8_t SD_TxDataBlock(const uint8_t *buff, uint8_t token)
{
    uint8_t resp;
    uint16_t i;

    if (!SD_WaitReady(500)) return 0;

    SPI_TxRxByte(token);

    if (token != 0xFD) {
        for (i = 0; i < 512; i++) {
            SPI_TxRxByte(buff[i]);
        }

        /* dummy CRC */
        SPI_TxRxByte(0xFF);
        SPI_TxRxByte(0xFF);

        resp = SPI_TxRxByte(0xFF);
        if ((resp & 0x1F) != 0x05) {
            return 0;
        }
    }

    return 1;
}

/* =========================
   解析 CSD 获取扇区数量
   这里只做常见 v2.0 卡
   不够时返回错误
   ========================= */
static SD_Result_t SD_ReadCSD(uint8_t *csd)
{
    if (SD_SendCmd(CMD9, 0) != 0x00) {
        SD_Deselect();
        return SD_RES_ERROR;
    }

    if (!SD_RxDataBlock(csd, 16)) {
        SD_Deselect();
        return SD_RES_ERROR;
    }

    SD_Deselect();
    return SD_RES_OK;
}

uint8_t SD_SPI_IsInitialized(void)
{
    return g_sd_initialized;
}

/* =========================
   初始化 SD 卡
   ========================= */
   
// SD_Result_t SD_SPI_InitCard(void)
// {
//     uint8_t i;
//     uint8_t r1;
//     uint8_t ocr[4];

//     g_sd_initialized = 0;
//     g_sd_type = SD_TYPE_UNKNOWN;

//     SD_CS_HIGH();
//     SD_SPI_SetSpeedLow();
//     HAL_Delay(10);

//     /* 上电后先送至少 74 个时钟 */
//     // for (i = 0; i < 10; i++) {
//     //     SPI_TxRxByte(0xFF);
//     // }
//     SD_SendDummyClocks(10);

//     /* CMD0 -> idle state */
//     i = 100;
//     do {
//         r1 = SD_SendCmd(CMD0, 0);
//     } while ((r1 != 0x01) && --i);

//     if (r1 != 0x01) {
//         SD_Deselect();
//         return SD_RES_ERROR;
//     }

//     /* CMD8 检查是否 SD v2 */
//     r1 = SD_SendCmd(CMD8, 0x1AA);
//     if (r1 == 0x01) {
//         for (i = 0; i < 4; i++) {
//             ocr[i] = SPI_TxRxByte(0xFF);
//         }

//         if ((ocr[2] == 0x01) && (ocr[3] == 0xAA)) {
//             uint32_t tickstart = HAL_GetTick();
//             do {
//                 r1 = SD_SendCmd(ACMD41, 1UL << 30);
//             } while ((r1 != 0x00) && ((HAL_GetTick() - tickstart) < 1000));

//             if (r1 == 0x00) {
//                 if (SD_SendCmd(CMD58, 0) == 0x00) {
//                     for (i = 0; i < 4; i++) {
//                         ocr[i] = SPI_TxRxByte(0xFF);
//                     }

//                     if (ocr[0] & 0x40) {
//                         g_sd_type = SD_TYPE_SDHC;
//                     } else {
//                         g_sd_type = SD_TYPE_SDSC;
//                     }
//                 }
//             }
//         }
//     } else {
//         /* 老卡流程 */
//         uint32_t tickstart = HAL_GetTick();
//         do {
//             r1 = SD_SendCmd(ACMD41, 0);
//         } while ((r1 != 0x00) && ((HAL_GetTick() - tickstart) < 1000));

//         if (r1 != 0x00) {
//             tickstart = HAL_GetTick();
//             do {
//                 r1 = SD_SendCmd(CMD1, 0);
//             } while ((r1 != 0x00) && ((HAL_GetTick() - tickstart) < 1000));
//         }

//         if (r1 == 0x00) {
//             g_sd_type = SD_TYPE_SDSC;
//             if (SD_SendCmd(CMD16, 512) != 0x00) {
//                 SD_Deselect();
//                 return SD_RES_ERROR;
//             }
//         }
//     }

//     SD_Deselect();

//     if (g_sd_type == SD_TYPE_UNKNOWN) {
//         return SD_RES_ERROR;
//     }

//     SD_SPI_SetSpeedHigh();
//     g_sd_initialized = 1;
//     return SD_RES_OK;
// }

SD_Result_t SD_SPI_InitCard(void)
{
    uint8_t i;
    uint8_t r1;
    uint8_t ocr[4];

    sd_dbg_step = 1;
    g_sd_initialized = 0;
    g_sd_type = SD_TYPE_UNKNOWN;

    SD_CS_HIGH();
    SD_SPI_SetSpeedLow();
    HAL_Delay(50);

    sd_dbg_step = 2;
    SD_SendDummyClocks(10);

    sd_dbg_step = 3;
    i = 100;
    do {
        r1 = SD_SendCmd(CMD0, 0);
        sd_dbg_r1_cmd0 = r1;
    } while ((r1 != 0x01) && --i);

    if (r1 != 0x01) {
        sd_dbg_step = 0xE0;
        SD_Deselect();
        return SD_RES_ERROR;
    }

    sd_dbg_step = 4;
    r1 = SD_SendCmd(CMD8, 0x1AA);
    sd_dbg_r1_cmd8 = r1;

    if (r1 == 0x01) {
        for (i = 0; i < 4; i++) {
            ocr[i] = SPI_TxRxByte(0xFF);
            sd_dbg_ocr[i] = ocr[i];
        }

        if ((ocr[2] == 0x01) && (ocr[3] == 0xAA)) {
            uint32_t tickstart = HAL_GetTick();
            sd_dbg_step = 5;
            do {
                r1 = SD_SendCmd(ACMD41, 1UL << 30);
                sd_dbg_r1_acmd41 = r1;
            } while ((r1 != 0x00) && ((HAL_GetTick() - tickstart) < 1000));

            if (r1 == 0x00) {
                sd_dbg_step = 6;
                if (SD_SendCmd(CMD58, 0) == 0x00) {
                    sd_dbg_r1_cmd58 = 0x00;
                    for (i = 0; i < 4; i++) {
                        ocr[i] = SPI_TxRxByte(0xFF);
                        sd_dbg_ocr[i] = ocr[i];
                    }

                    if (ocr[0] & 0x40) {
                        g_sd_type = SD_TYPE_SDHC;
                    } else {
                        g_sd_type = SD_TYPE_SDSC;
                    }
                } else {
                    sd_dbg_r1_cmd58 = 0xFF;
                }
            } else {
                sd_dbg_step = 0xE1;
            }
        } else {
            sd_dbg_step = 0xE2;
        }
    } else {
        uint32_t tickstart = HAL_GetTick();
        sd_dbg_step = 7;
        do {
            r1 = SD_SendCmd(ACMD41, 0);
            sd_dbg_r1_acmd41 = r1;
        } while ((r1 != 0x00) && ((HAL_GetTick() - tickstart) < 1000));

        if (r1 != 0x00) {
            tickstart = HAL_GetTick();
            sd_dbg_step = 8;
            do {
                r1 = SD_SendCmd(CMD1, 0);
            } while ((r1 != 0x00) && ((HAL_GetTick() - tickstart) < 1000));
        }

        if (r1 == 0x00) {
            g_sd_type = SD_TYPE_SDSC;
            if (SD_SendCmd(CMD16, 512) != 0x00) {
                sd_dbg_step = 0xE3;
                SD_Deselect();
                return SD_RES_ERROR;
            }
        } else {
            sd_dbg_step = 0xE4;
        }
    }

    SD_Deselect();

    if (g_sd_type == SD_TYPE_UNKNOWN) {
        sd_dbg_step = 0xEF;
        return SD_RES_ERROR;
    }

    sd_dbg_step = 9;
    SD_SPI_SetSpeedHigh();
    g_sd_initialized = 1;

    sd_dbg_step = 10;
    return SD_RES_OK;
}

/* =========================
   单块/多块读
   先实现单块，count>1 时循环单块
   ========================= */
SD_Result_t SD_SPI_ReadBlocks(uint8_t *buff, uint32_t sector, uint32_t count)
{
    uint32_t addr;
    uint32_t i;

    if (!g_sd_initialized) return SD_RES_NOTRDY;
    if ((buff == NULL) || (count == 0)) return SD_RES_ERROR;

    for (i = 0; i < count; i++) {
        addr = sector + i;
        if (g_sd_type != SD_TYPE_SDHC) {
            addr *= 512;
        }

        if (SD_SendCmd(CMD17, addr) != 0x00) {
            SD_Deselect();
            return SD_RES_ERROR;
        }

        if (!SD_RxDataBlock(buff + i * 512, 512)) {
            SD_Deselect();
            return SD_RES_ERROR;
        }

        SD_Deselect();
    }

    return SD_RES_OK;
}

/* =========================
   单块/多块写
   先实现单块，count>1 时循环单块
   ========================= */
SD_Result_t SD_SPI_WriteBlocks(const uint8_t *buff, uint32_t sector, uint32_t count)
{
    uint32_t addr;
    uint32_t i;

    if (!g_sd_initialized) return SD_RES_NOTRDY;
    if ((buff == NULL) || (count == 0)) return SD_RES_ERROR;

    for (i = 0; i < count; i++) {
        addr = sector + i;
        if (g_sd_type != SD_TYPE_SDHC) {
            addr *= 512;
        }

        if (SD_SendCmd(CMD24, addr) != 0x00) {
            SD_Deselect();
            return SD_RES_ERROR;
        }

        if (!SD_TxDataBlock(buff + i * 512, 0xFE)) {
            SD_Deselect();
            return SD_RES_ERROR;
        }

        if (!SD_WaitReady(500)) {
            SD_Deselect();
            return SD_RES_TIMEOUT;
        }

        SD_Deselect();
    }

    return SD_RES_OK;
}

/* =========================
   信息接口
   ========================= */
SD_Result_t SD_SPI_GetSectorSize(uint16_t *size)
{
    if (size == NULL) return SD_RES_ERROR;
    *size = 512;
    return SD_RES_OK;
}

SD_Result_t SD_SPI_GetBlockSize(uint32_t *size)
{
    if (size == NULL) return SD_RES_ERROR;
    *size = 8;
    return SD_RES_OK;
}

SD_Result_t SD_SPI_GetSectorCount(uint32_t *count)
{
    uint8_t csd[16];

    if (count == NULL) return SD_RES_ERROR;
    if (!g_sd_initialized) return SD_RES_NOTRDY;

    if (SD_ReadCSD(csd) != SD_RES_OK) {
        return SD_RES_ERROR;
    }

    /* CSD Version 2.0 */
    if ((csd[0] >> 6) == 1) {
        uint32_t c_size;
        c_size = ((uint32_t)(csd[7] & 0x3F) << 16) |
                 ((uint32_t)csd[8] << 8) |
                 csd[9];
        *count = (c_size + 1) * 1024;
        return SD_RES_OK;
    }

    /* 老卡简单计算 */
    {
        uint32_t c_size;
        uint8_t read_bl_len;
        uint8_t c_size_mult;
        uint32_t blocknr;
        uint32_t block_len;

        read_bl_len = csd[5] & 0x0F;
        c_size = ((uint32_t)(csd[6] & 0x03) << 10) |
                 ((uint32_t)csd[7] << 2) |
                 ((csd[8] & 0xC0) >> 6);
        c_size_mult = ((csd[9] & 0x03) << 1) |
                      ((csd[10] & 0x80) >> 7);

        blocknr = (c_size + 1) * (1UL << (c_size_mult + 2));
        block_len = (1UL << read_bl_len);

        *count = (blocknr * block_len) / 512;
        return SD_RES_OK;
    }
}