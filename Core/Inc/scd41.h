/**
 * @file    scd41.h
 * @brief   SCD41二氧化碳传感器模块头文件
 * @author  Microwave Oven
 * @date    2026-03-05
 * @version 1.0
 *
 * 该模块提供SCD41传感器的基本控制函数，包括获取CO2浓度等功能。
 * SCD41传感器连接到STM32F4的I2C1接口。
 */

#ifndef __SCD41_H
#define __SCD41_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define SCD41_I2C_ADDR_62 0x62

int16_t SCD41_Init(void);
uint16_t SCD41_Read_CO2(void);

#endif //! __SCD41_H 