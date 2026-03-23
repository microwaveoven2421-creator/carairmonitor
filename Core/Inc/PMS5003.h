#ifndef __PMS5003_H
#define __PMS5003_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* ====================== 错误码定义 ====================== */
#define PMS5003_ERR_NONE        0   // 无错误
#define PMS5003_ERR_CHECKSUM    -1  // 校验和错误
#define PMS5003_ERR_PARAM       -2  // 参数错误
#define PMS5003_ERR_UART        -3  // 串口发送错误
#define PMS5003_ERR_NO_DATA     -4  // 无有效数据

/* ====================== 全局变量声明 ====================== */
extern uint16_t g_PM1_0_Value;
extern uint16_t g_PM2_5_Value;
extern uint16_t g_PM10_0_Value;

/* ====================== 函数声明 ====================== */
int16_t PMS5003_Init(void);
int16_t PMS5003_ReceiveData(uint8_t data);
int16_t PMS5003_Read_PM_Value(void);
int16_t PMS5003_Send_PM_Value(UART_HandleTypeDef* huart, uint8_t *pData, uint8_t pm_type);
void read_pms5003(UART_HandleTypeDef* huart, uint8_t *pData, uint8_t pm_type);

#endif // !__PMS5003_H