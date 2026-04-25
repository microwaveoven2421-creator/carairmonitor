/**
 * @file    buzzer.h
 * @brief   无源蜂鸣器报警控制接口。
 * @details 提供蜂鸣器初始化、启停和传感器报警播放相关函数声明。
 * @author  Microwave Oven
 */

/**
 * @file    buzzer.h
 * @brief   无源蜂鸣器报警模块头文件
 * @author  Microwave Oven
 * @date    2026-03-05
 * @version 1.0
 *
 * 该模块提供无源蜂鸣器的基本控制函数，包括开、关和报警功能。
 * 蜂鸣器连接到STM32F4的TIM3通道4（PB1）用于控制蜂鸣器的PWM输出，实现不同频率的报警声。
 * 报警函数支持指定持续时间，超过阈值时自动触发报警.
 */

#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

extern TIM_HandleTypeDef htim3;

/* ====================== 配置项 ====================== */
#define BUZZER_TIM_HANDLE    &htim3          // 蜂鸣器定时器句柄
#define BUZZER_TIM_CHANNEL   TIM_CHANNEL_4   // 蜂鸣器定时器通道（PB1对应CH4）
#define BUZZER_DEFAULT_FREQ  1000            // 默认报警频率（1kHz）
#define BUZZER_DEFAULT_DUTY  50              // 默认占空比（50%）


/* ====================== 报警模式枚举 ====================== */
typedef enum {
    BUZZER_ALARM_ONCE,       // 单次报警（响指定时长后停）
    BUZZER_ALARM_LOOP,       // 循环报警（响-停循环，需手动停止）
    BUZZER_ALARM_BEEP        // 短鸣提示（100ms，用于轻提示）
} Buzzer_Alarm_Mode;


/* ====================== 通用API接口 ====================== */
/**
 * @brief  蜂鸣器初始化（配置默认频率和占空比）
 * @param  freq: 蜂鸣器频率（Hz），传0则用默认值
 * @retval None
 */
void Buzzer_Init(uint32_t freq);

/**
 * @brief  蜂鸣器通用报警函数（核心）
 * @param  duration_ms: 单次报警时长（ms），循环模式下为“响”的时长
 * @param  interval_ms: 循环模式下“停”的时长（单次模式传0）
 * @param  mode: 报警模式（单次/循环/短鸣）
 * @retval None
 */
void Buzzer_Alarm(uint32_t duration_ms, uint32_t interval_ms, Buzzer_Alarm_Mode mode);

/**
 * @brief  停止蜂鸣器（用于循环报警的手动停止）
 * @retval None
 */
void Buzzer_Stop(void);

/**
 * @brief  传感器阈值报警（一键调用，最常用）
 * @param  sensor_value: 传感器当前值
 * @param  threshold: 报警阈值
 * @param  duration_ms: 报警时长
 * @retval None
 */
void Buzzer_Sensor_Alarm(uint32_t sensor_value, uint32_t threshold, uint32_t duration_ms);

#endif // !__BUZZER_H
