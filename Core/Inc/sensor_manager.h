/**
 * @file    sensor_manager.h
 * @brief   传感器管理模块头文件
 * @author  Microwave Oven
 * @date    2026-03-06
 * @version 1.0
 *
 * 该模块提供传感器的基本控制函数，包括数据采集和报警功能。
 * 模块引脚数据：
 * - SCD41:     I2C1 (SCL: PB6, SDA: PB7)
 * - OLED:      I2C1 (SCL: PB6, SDA: PB7)
 * - SHT31:     I2C2 (SCL: PB10, SDA: PB11)
 * - 调试串口： UART1 (TX: PA9, RX: PA10)
 * - PMS5003:   UART2 (TX: PA2, RX: PA3)
 * - JDY23蓝牙: UART6 (TX: PC6, RX: PC7)
 * - MQ3:       ADC1 (Channel 0, PA0)
 * - 蜂鸣器:    TIM3_CH4 (PB1)
 * - SD卡：     SPI3 (CS:PC0, SCK:PC10, MISO:PC11, MOSI:PC12)
 * 
 */

#ifndef __SENSOR_MANAGER_H
#define __SENSOR_MANAGER_H

#include "main.h"
#include <stdint.h>

extern I2C_HandleTypeDef hi2c1;
extern I2C_HandleTypeDef hi2c2;
extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;

/* ====================== 系统配置参数（可在这里统一配置） ====================== */
#define COLLECT_INTERVAL_MS    5000    // 数据采集间隔：必须>=5秒,单次测量需要5秒完成
#define TEMP_DISPLAY_PRECISION 2       // 温度显示精度（小数点后2位）
#define HUM_DISPLAY_PRECISION  2       // 湿度显示精度（小数点后2位）
#define UART_TX_TIMEOUT_MS     100     // 串口发送超时时间
#define SCD41_MEASURE_MODE     0       // 0=周期性模式（推荐），1=单次模式

/* ====================== 硬件配置 ====================== */
#define PMS5003_UART_HANDLE    huart2  // PMS5003使用串口2
#define DEBUG_UART_HANDLE      huart1  // 调试打印使用串口1
#define SHT31_I2C_HANDLE       hi2c2   // SHT31使用I2C2

/* ====================== 报警阈值配置 ====================== */
#define MQ3_ALARM_THRESHOLD    1.0f    // 酒精报警阈值
#define CO2_ALARM_THRESHOLD    1500    // CO2报警阈值（ppm）
#define PM25_ALARM_THRESHOLD   75      // PM2.5报警阈值（μg/m³）
#define TEMP_ALARM_LOW         0.0f   // 温度低阈值（℃）
#define TEMP_ALARM_HIGH        35.0f   // 温度高阈值（℃）
#define HUM_ALARM_LOW          15.0f   // 湿度低阈值（%）
#define HUM_ALARM_HIGH         70.0f   // 湿度高阈值（%）
#define ALARM_DURATION         1000    // 统一报警时长：1秒

/* ====================== 全局变量声明（如需外部访问） ====================== */
extern int32_t g_temp_m_deg_c;        // 全局温度值（毫摄氏度）
extern int32_t g_hum_m_percent_rh;    // 全局湿度值（毫百分比湿度）
extern uint16_t g_PM1_0_Value;        // PM1.0值
extern uint16_t g_PM2_5_Value;        // PM2.5值
extern uint16_t g_PM10_0_Value;       // PM10值

/* ====================== 函数声明 ====================== */
/**
 * @brief  传感器系统全局初始化（包含所有传感器+蜂鸣器）
 * @retval None
 */
void Sensor_Manager_Init(void);

/**
 * @brief  传感器数据采集+报警（温湿度+酒精+CO2+PM2.5）
 * @retval None
 */
void Sensor_Manager_Collect_Alarm(void);

/**
 * @brief  获取系统时间戳
 * @retval None
 * @note   基于HAL_GetTick，HSE 8MHz下无误差，格式为 "HH:MM:SS"
 */
void Get_System_Time_Stamp(char *time_buf, uint8_t buf_len);

#endif //! __SENSOR_MANAGER_H 