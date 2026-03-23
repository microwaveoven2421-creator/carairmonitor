/**
 * @file    sht31.h
 * @brief   SHT31温湿度传感器模块头文件
 * @author  Microwave Oven
 * @date    2026-03-05
 * @version 2.0
 *
 * 该模块提供SHT31传感器的基本控制函数，包括获取温度和湿度等功能。
 * SHT31传感器连接到STM32F4的I2C2接口。
 */

#ifndef __SHT31_H
#define __SHT31_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

#define SHT31_I2C_ADDR_44 0x44

int16_t SHT31_Init(void);
uint16_t SHT31_Read_Temp_Hum(void);
int32_t SHT31_Read_Temperature(void);
int32_t SHT31_Read_Humidity(void);

#endif //! __SHT31_H 