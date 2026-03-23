/**
 * @file    sht3X_i2c.h
 * @brief   SHT3X系列传感器模块头文件
 * @author  Microwave Oven
 * @date    2026-03-05
 * @version 1.0
 *
 * 该模块提供SHT3X传感器的I2C通信接口函数，包括初始化和数据读取等功能。
 * SHT3X传感器连接到STM32F4的I2C接口。
 */

#ifndef SHT3X_I2C_H
#define SHT3X_I2C_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f4xx_hal.h"
#include "sensirion_config.h"
#include <stdint.h>

extern I2C_HandleTypeDef hi2c2; // 在main.c中定义的I2C句柄
/**
 * @brief 硬件I2C配置参数
 */
#define SHT3X_I2C_HANDLE        hi2c2               // 硬件I2C句柄（需与main.c中定义的一致）
#define SHT3X_I2C_INSTANCE      I2C2               // 使用的I2C外设
#define SHT3X_I2C_SCL_PORT      GPIOB              // SCL引脚端口
#define SHT3X_I2C_SCL_PIN       GPIO_PIN_10        // SCL引脚
#define SHT3X_I2C_SDA_PORT      GPIOB              // SDA引脚端口
#define SHT3X_I2C_SDA_PIN       GPIO_PIN_11        // SDA引脚
#define SHT3X_I2C_BAUDRATE      100000U            // I2C通信波特率（100KHz）

#define SHT3X_I2C_ADDR_44       0x44 //默认地址，ADDR引脚接到逻辑低电平
#define SHT3X_I2C_ADDR_45       0x45 //ADDR引脚接逻辑高电平

// 指令集
typedef enum {
    SHT3X_MEASURE_SINGLE_SHOT_HIGHREP_STRETCH_CMD_ID = 0x2C06, //单次读取启动时钟延伸，高重复性
    SHT3X_MEASURE_SINGLE_SHOT_MEDREP_STRETCH_CMD_ID = 0x2C0D,  //单次读取启动时钟延伸，中重复性
    SHT3X_MEASURE_SINGLE_SHOT_LOWREP_STRETCH_CMD_ID = 0x2C10,  //单次读取启动时钟延伸，低重复性
    
    SHT3X_MEASURE_SINGLE_SHOT_HIGHREP_CMD_ID = 0x2400,         //单次读取不启动时钟延伸，高重复性
    SHT3X_MEASURE_SINGLE_SHOT_MEDREP_CMD_ID = 0x240B,          //单次读取不启动时钟延伸，中重复性
    SHT3X_MEASURE_SINGLE_SHOT_LOWREP_CMD_ID = 0x2416,          //单次读取不启动时钟延伸，低重复性
   
    SHT3X_MEASURE_PERIODIC_0_5MPS_HIGHREP_CMD_ID = 0x2032,    //周期数据采集模式（每秒0.5次，高重复性）
    SHT3X_MEASURE_PERIODIC_0_5MPS_MEDREP_CMD_ID = 0x2024,     //周期数据采集模式（每秒0.5次，中重复性）
    SHT3X_MEASURE_PERIODIC_0_5MPS_LOWREP_CMD_ID = 0x202F,     //周期数据采集模式（每秒0.5次，低重复性）
    
    SHT3X_MEASURE_PERIODIC_1MPS_HIGHREP_CMD_ID = 0x2130,      //周期数据采集模式（每秒1次，高重复性）
    SHT3X_MEASURE_PERIODIC_1MPS_MEDREP_CMD_ID = 0x2126,       //周期数据采集模式（每秒1次，中重复性）
    SHT3X_MEASURE_PERIODIC_1MPS_LOWREP_CMD_ID = 0x212D,       //周期数据采集模式（每秒1次，低重复性）
    
    SHT3X_MEASURE_PERIODIC_2MPS_HIGHREP_CMD_ID = 0x2236,      //周期数据采集模式（每秒2次，高重复性）
    SHT3X_MEASURE_PERIODIC_2MPS_MEDREP_CMD_ID = 0x2220,       //周期数据采集模式（每秒2次，中重复性）
    SHT3X_MEASURE_PERIODIC_2MPS_LOWREP_CMD_ID = 0x222B,       //周期数据采集模式（每秒2次，低重复性）

    SHT3X_MEASURE_PERIODIC_4MPS_HIGHREP_CMD_ID = 0x2334,      //周期数据采集模式（每秒4次，高重复性）
    SHT3X_MEASURE_PERIODIC_4MPS_MEDREP_CMD_ID = 0x2322,       //周期数据采集模式（每秒4次，中重复性）
    SHT3X_MEASURE_PERIODIC_4MPS_LOWREP_CMD_ID = 0x2329,       //周期数据采集模式（每秒4次，低重复性）

    SHT3X_MEASURE_PERIODIC_10MPS_HIGHREP_CMD_ID = 0x2737,     //周期数据采集模式（每秒10次，高重复性）
    SHT3X_MEASURE_PERIODIC_10MPS_MEDREP_CMD_ID = 0x2721,      //周期数据采集模式（每秒10次，中重复性）
    SHT3X_MEASURE_PERIODIC_10MPS_LOWREP_CMD_ID = 0x272A,      //周期数据采集模式（每秒10次，低重复性）
    
    SHT3X_FETCH_PERIODIC_MEASUREMENT_DATA_CMD_ID = 0xE000,    //读取周期测量数据命令
    SHT3X_MEASURE_PERIODIC_ART_CMD_ID = 0x2B32,               //激活ART（自动速率调整）功能
    SHT3X_STOP_PERIODIC_MEASUREMENT_CMD_ID = 0x3093,                              //停止周期数据采集模式
    SHT3X_SOFT_RESET_CMD_ID = 0x30A2,                         //软件复位命令
    SHT3X_GENERAL_CALL_CMD_ID = 0x0006,                       //广播重置命令
    SHT3X_HEATER_ENABLE_CMD_ID = 0x306D,                     //加热器开启命令
    SHT3X_HEATER_DISABLE_CMD_ID = 0x3066,                    //加热器关闭命令
    SHT3X_READ_OUT_OF_STATUS_REGISTER_CMD_ID = 0xF32D,       //读取状态寄存器命令
    SHT3X_CLEAR_STATUS_REGISTER_CMD_ID = 0x3041,             //清除状态寄存器命令
} SHT3X_CMD_ID;

// SHT3x系列不同型号
typedef enum {
    SHT3X_SENSOR_VARIANT_SHT30 = 0x0000,
    SHT3X_SENSOR_VARIANT_SHT31 = 0x0001,
} sht3x_sensor_variant;

/**
 * @brief 初始化驱动程序的I2C地址
 *
 * @param[in] i2c_address 使用的I2C地址
 */
void sht3x_init(uint8_t i2c_addr);

/**
 * @brief 启动周期测量模式
 *
 * 启动周期测量模式。信号测量持续时间为4/6/15毫秒（低/中/高重复性）
 * 
 * @note 此命令仅在空闲模式下可用
 *
 * @return 错误码 0表示成功，其他值表示错误
 */
uint16_t sht3x_start_periodic_measurement();

/**
 * @brief 停止周期测量模式
 *
 * 停止周期测量模式并返回空闲模式
 *
 * @note 此命令仅在周期测量模式下可用
 *
 * @return 错误码 0表示成功，其他值表示错误
 */
uint16_t sht3x_stop_periodic_measurement();

/**
 * @brief 读取温度和湿度的原始测量值
 *
 * 读取传感器输出。测量数据只能在每个信号更新间隔内读取一次，
 * 因为读取后缓冲区会被清空。如果缓冲区中没有可用数据，
 * 传感器将返回NACK。为避免NACK响应，可以调用get_data_ready_status
 * 来检查数据状态。如果用户对后续数据不感兴趣，I2C主设备可以在
 * 任何数据字节后通过NACK和STOP条件中止读取传输。
 *
 * @param[out] temperature 通过公式 (175 * value / 65535) - 45 转换为摄氏度
 * @param[out] humidity 通过公式 (100 * value / 65535) 转换为相对湿度百分比
 *
 * @return 错误码 0表示成功，其他值表示错误
 */
int16_t sht3x_read_measurement_raw(uint16_t* temperature, 
                                   uint16_t* humidity);

/**
 * @brief 读取传感器输出并转换为物理单位
 *
 * 详见 @ref sht3x_read_measurement_raw() 了解更多细节
 *
 * @param[out] temperature_m_deg_c 以毫摄氏度为单位（°C * 1000）
 * @param[out] humidity_m_percent_rh 以毫百分比相对湿度为单位（%RH * 1000）
 * @return 0表示成功，错误代码表示失败
 */
int16_t sht3x_read_measurement(int32_t* temperature_m_deg_c, 
                               int32_t* humidity_m_percent_rh);

/**
 * @brief 执行单次测量
 *
 * 发送单次测量命令并等待测量完成
 *
 * @return 错误码 0表示成功，其他值表示错误
 */
int16_t sht3x_measure_single_shot();

/**
 * @brief 执行软件复位
 *
 * 向传感器发送软件复位命令，使传感器恢复到初始状态
 *
 * @return 错误码 0表示成功，其他值表示错误
 */
int16_t sht3x_soft_reset();

#ifdef __cplusplus
}
#endif
#endif  // SHT3X_I2C_H