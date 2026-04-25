/**
 * @file    mq3.h
 * @brief   MQ-3 酒精传感器采样接口。
 * @details 声明原始 ADC 读取和平均 ADC 读取函数。
 * @author  Microwave Oven
 */

/**
 * @file    mq3.h
 * @brief   MQ3酒精探测器模块头文件
 * @author  Microwave Oven
 * @date    2026-03-05
 * @version 1.0
 *
 * 该模块提供MQ3传感器的基本控制函数，包括获取酒精浓度等功能。
 * MQ3传感器连接到STM32F4的ADC1通道0（PA0）。
 */

#ifndef __MQ3_H
#define __MQ3_H

#include "stm32f4xx_hal.h"

uint16_t MQ3_Get_AlcoholConcentration(uint32_t ch);
uint16_t MQ3_Get_AlcoholConcentration_Average(uint32_t ch, uint8_t times);

#endif // !__MQ3_H
