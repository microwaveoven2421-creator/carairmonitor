/**
 * @file    scd41.c
 * @brief   SCD41 二氧化碳传感器应用封装。
 * @details 初始化 SCD41 测量模式，并提供 CO2 浓度读取函数。
 * @author  Microwave Oven
 */

#include "scd41.h"
#include "scd4x_i2c.h"
#include "sensirion_i2c_hal.h"
#include "sensirion_common.h"
#include "main.h"
#include <inttypes.h>
#include <stdio.h>
#include <math.h>
#include "sensor_manager.h"//在这个文件里修改measure模式

// 宏定义：1000百帕 = 100000帕斯卡（气压设置用）
#define AMBIENT_PRESSURE_1000HPA  100000U
// 模式选择：0-周期性测量（推荐），1-单次测量
// #define SCD41_MEASURE_MODE        0  

/* ====================== 周期性测量模式 ====================== */
/**
 * @brief  SCD41初始化（周期性测量模式）
 * @note   官方推荐稳定模式，数据每5秒更新一次，集成1000百帕气压设置
 * @retval 0-成功，非0-错误码
 */
int16_t SCD41_Init_Periodic(void) {
    int16_t error = NO_ERROR;
    uint16_t serial_number[3] = {0};
    uint32_t set_pressure = 0;

    // 1. 初始化I2C硬件层
    sensirion_i2c_hal_init();
    scd4x_init(SCD41_I2C_ADDR_62);
    sensirion_i2c_hal_sleep_usec(50000); // 50ms稳定延时

    // 2. 传感器状态重置
    error = scd4x_wake_up();
    if (error != NO_ERROR) {
        printf("SCD41 Wake Up Error: %d\r\n", error);
        return error;
    }
    sensirion_i2c_hal_sleep_usec(10000); // 唤醒后延时10ms

    error = scd4x_stop_periodic_measurement(); // 停止残留测量
    if (error != NO_ERROR) {
        printf("SCD41 Stop Measurement Error: %d\r\n", error);
        return error;
    }
    sensirion_i2c_hal_sleep_usec(500000); // 500ms停止延时

    error = scd4x_reinit(); // 重新初始化
    if (error != NO_ERROR) {
        printf("SCD41 Reinit Error: %d\r\n", error);
        return error;
    }
    sensirion_i2c_hal_sleep_usec(20000); // 20ms稳定延时

    // 3. 读取序列号
    error = scd4x_get_serial_number(serial_number, 3);
    if (error != NO_ERROR) {
        printf("SCD41 Read Serial Error: %d\r\n", error);
        return error;
    }
    uint64_t serial_as_int = 0;
    sensirion_common_to_integer((uint8_t*)serial_number, (uint8_t*)&serial_as_int, LONG_INTEGER, 6);
    printf("SCD41 Serial Number: 0x%" PRIx64 "\r\n", serial_as_int);

    // 4. 设置1000百帕环境气压
    error = scd4x_set_ambient_pressure(AMBIENT_PRESSURE_1000HPA);
    if (error != NO_ERROR) {
        printf("SCD41 Set Pressure Error: %d\r\n", error);
        return error;
    }
    // 验证气压设置结果
    error = scd4x_get_ambient_pressure(&set_pressure);
    if (error != NO_ERROR) {
        printf("SCD41 Get Pressure Error: %d\r\n", error);
        return error;
    }
    printf("SCD41 Ambient Pressure Set: %lu Pa (%.1f hPa)\r\n", 
           set_pressure, (float)set_pressure / 100.0);

    // 5. 启动周期性测量（核心）
    error = scd4x_start_periodic_measurement();
    if (error != NO_ERROR) {
        printf("SCD41 Start Periodic Measurement Error: %d\r\n", error);
        return error;
    }

    printf("SCD41 Init (Periodic Mode) Success! Wait 5s for first data...\r\n");
    //HAL_Delay(5000); // 等待首次数据就绪
    return NO_ERROR;
}

/**
 * @brief  SCD41读取CO2（周期性测量模式）
 * @note   需确保采集间隔≥5秒，自动等待数据就绪
 * @retval CO2浓度值（ppm），0-读取失败/数据无效
 */
uint16_t SCD41_Read_CO2_Periodic(void) {
    bool data_ready = false;
    uint16_t co2 = 0;
    int32_t temp_mc, hum_mrh;
    int16_t error = NO_ERROR;
    
    error = scd4x_get_data_ready_status(&data_ready);
    
    if (error != NO_ERROR || !data_ready) {
        // 数据没准备好，直接返回0，等待下一次调用时再尝试读取（周期性测量模式会持续更新数据）
        // printf("SCD41 data not ready (will retry next cycle)\r\n"); // 调试用
        return 0;
    }

    // 读取测量数据
    error = scd4x_read_measurement(&co2, &temp_mc, &hum_mrh);
    if (error != NO_ERROR) {
        printf("SCD41 (Periodic) Read Measurement Error: %d\r\n", error);
        return 0;
    }

    // 过滤无效值（正常范围：0~20000ppm）
    if (co2 == 0 || co2 > 20000) {
        printf("SCD41 (Periodic) Invalid CO2 Value: %d\r\n", co2);
        return 0;
    }

    return co2;
}

/* ====================== 单次测量模式 ====================== */
/**
 * @brief  SCD41初始化（单次测量模式）
 * @note   仅初始化I2C和传感器状态，不启动任何测量，集成1000百帕气压设置
 * @retval 0-成功，非0-错误码
 */
int16_t SCD41_Init_SingleShot(void) {
    int16_t error = NO_ERROR;
    uint16_t serial_number[3] = {0};
    uint32_t set_pressure = 0;

    // 1. 初始化I2C硬件层
    sensirion_i2c_hal_init();
    scd4x_init(SCD41_I2C_ADDR_62);
    sensirion_i2c_hal_sleep_usec(50000); // 50ms稳定延时

    // 2. 传感器状态重置
    error = scd4x_wake_up();
    if (error != NO_ERROR) {
        printf("SCD41 Wake Up Error: %d\r\n", error);
        return error;
    }
    sensirion_i2c_hal_sleep_usec(10000); // 唤醒后延时10ms

    error = scd4x_stop_periodic_measurement(); // 停止残留测量
    if (error != NO_ERROR) {
        printf("SCD41 Stop Measurement Error: %d\r\n", error);
        return error;
    }
    sensirion_i2c_hal_sleep_usec(500000); // 500ms停止延时

    error = scd4x_reinit(); // 重新初始化
    if (error != NO_ERROR) {
        printf("SCD41 Reinit Error: %d\r\n", error);
        return error;
    }
    sensirion_i2c_hal_sleep_usec(20000); // 20ms稳定延时

    // 3. 读取序列号
    error = scd4x_get_serial_number(serial_number, 3);
    if (error != NO_ERROR) {
        printf("SCD41 Read Serial Error: %d\r\n", error);
        return error;
    }
    uint64_t serial_as_int = 0;
    sensirion_common_to_integer((uint8_t*)serial_number, (uint8_t*)&serial_as_int, LONG_INTEGER, 6);
    printf("SCD41 Serial Number: 0x%" PRIx64 "\r\n", serial_as_int);

    // 4. 设置1000百帕环境气压
    error = scd4x_set_ambient_pressure(AMBIENT_PRESSURE_1000HPA);
    if (error != NO_ERROR) {
        printf("SCD41 Set Pressure Error: %d\r\n", error);
        return error;
    }
    // 验证气压设置结果
    error = scd4x_get_ambient_pressure(&set_pressure);
    if (error != NO_ERROR) {
        printf("SCD41 Get Pressure Error: %d\r\n", error);
        return error;
    }
    printf("SCD41 Ambient Pressure Set: %lu Pa (%.1f hPa)\r\n", 
           set_pressure, (float)set_pressure / 100.0);

    printf("SCD41 Init (Single Shot Mode) Success!\r\n");
    return NO_ERROR;
}

/**
 * @brief  SCD41读取CO2（单次测量模式）
 * @note   每次读取都会触发新测量，阻塞6秒等待结果，集成气压补偿
 * @retval CO2浓度值（ppm），0-读取失败/数据无效
 */
uint16_t SCD41_Read_CO2_SingleShot(void) {
    uint16_t co2 = 0;
    int32_t temp_mc, hum_mrh;
    int16_t error = NO_ERROR;
    bool data_ready = false;

    // 1. 唤醒传感器
    error = scd4x_wake_up();
    if (error != NO_ERROR) {
        printf("SCD41 (Single Shot) Wake Up Error: %d\r\n", error);
        return 0;
    }
    sensirion_i2c_hal_sleep_usec(20000); // 20ms稳定延时

    // 2. 停止残留的周期性测量
    error = scd4x_stop_periodic_measurement();
    if (error != NO_ERROR) {
        printf("SCD41 (Single Shot) Stop Measurement Error: %d\r\n", error);
        return 0;
    }
    sensirion_i2c_hal_sleep_usec(1000000); // 1秒停止延时

    // 3. 重新设置1000百帕气压（确保每次测量都生效）
    error = scd4x_set_ambient_pressure(AMBIENT_PRESSURE_1000HPA);
    if (error != NO_ERROR) {
        printf("SCD41 (Single Shot) Set Pressure Error: %d\r\n", error);
        return 0;
    }

    // 4. 触发单次测量
    error = scd4x_measure_single_shot();
    if (error != NO_ERROR) {
        printf("SCD41 (Single Shot) Trigger Measure Error: %d\r\n", error);
        return 0;
    }

    // 5. 等待测量完成（硬件要求≥5秒，这里设6秒更稳定）
    //printf("SCD41 (Single Shot) Measuring... Wait 6s\r\n");
    HAL_Delay(6000);

    // 6. 检查数据是否就绪
    error = scd4x_get_data_ready_status(&data_ready);
    if (error != NO_ERROR || !data_ready) {
        printf("SCD41 (Single Shot) Data Not Ready! Err=%d, Ready=%d\r\n", error, data_ready);
        // 强制读取（兼容部分固件不更新ready标志的情况）
        error = scd4x_read_measurement(&co2, &temp_mc, &hum_mrh);
        if (error != NO_ERROR) {
            return 0;
        }
    } else {
        // 读取测量数据
        error = scd4x_read_measurement(&co2, &temp_mc, &hum_mrh);
        if (error != NO_ERROR) {
            printf("SCD41 (Single Shot) Read Measurement Error: %d\r\n", error);
            return 0;
        }
    }

    // 7. 过滤无效值（正常范围：0~20000ppm）
    if (co2 == 0 || co2 > 20000) {
        printf("SCD41 (Single Shot) Invalid CO2 Value: %d\r\n", co2);
        return 0;
    }

    return co2;
}

/* ====================== 通用初始化函数（根据宏自动选择模式） ====================== */
int16_t SCD41_Init(void) {
#if SCD41_MEASURE_MODE == 0
    return SCD41_Init_Periodic(); // 周期性模式
#elif SCD41_MEASURE_MODE == 1
    return SCD41_Init_SingleShot(); // 单次模式
#else
    printf("SCD41 Measure Mode Error! Set SCD41_MEASURE_MODE to 0 or 1\r\n");
    return -1;
#endif
}

/* ====================== 通用读取函数（根据宏自动选择模式） ====================== */
uint16_t SCD41_Read_CO2(void) {
#if SCD41_MEASURE_MODE == 0
    return SCD41_Read_CO2_Periodic(); // 周期性模式
#elif SCD41_MEASURE_MODE == 1
    return SCD41_Read_CO2_SingleShot(); // 单次模式
#else
    printf("SCD41 Measure Mode Error! Set SCD41_MEASURE_MODE to 0 or 1\r\n");
    return 0;
#endif
}
