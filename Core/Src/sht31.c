#include "sht31.h"
#include "sht3x_i2c.h"
#include "sensirion_i2c_hal.h"
#include "sensirion_common.h"
#include "main.h"
#include <inttypes.h>
#include <stdio.h>
#include <stdint.h>

#define NO_ERROR 0
#define I2C_TRANSMIT_ERROR -1
#define I2C_RECEIVE_ERROR -2
#define CRC_CHECK_ERROR -3

// 全局变量：存储温度和湿度的毫单位值（供读取函数使用）
static int32_t g_temp_m_deg_c = 0;    // 毫摄氏度 (°C * 1000)
static int32_t g_hum_m_percent_rh = 0;// 毫百分比湿度 (%RH * 1000)

/**
 * @brief SHT31传感器初始化函数
 * @note 参考SCD41_Init实现风格，包含复位、状态清理、周期测量启动等流程
 * @return 0-成功，非0-错误码
 */
int16_t SHT31_Init(void) {
    int16_t error = NO_ERROR;

    // 1. 初始化I2C硬件层（复用Sensirion的I2C硬件层初始化）
    sensirion_i2c_hal_init();
    
    // 2. 初始化传感器I2C地址（默认0x44）
    sht3x_init(SHT3X_I2C_ADDR_44);
    sensirion_i2c_hal_sleep_usec(30000); // 上电稳定延时30ms

    // 3. 停止可能的周期测量（核心：确保进入空闲模式，替代复位的作用）
    error = sht3x_stop_periodic_measurement();
    if (error != NO_ERROR) {
        printf("SHT31 Stop Measurement Error: %d\r\n", error);
        // 非致命错误：继续执行，避免卡死（首次上电时可能无测量需要停止）
        printf("Warning: No active measurement to stop, continue init...\r\n");
        error = NO_ERROR; // 重置错误码，继续初始化
    }

    // 4. 启动周期测量模式（默认：10MPS，Medium Repeatability）
    error = sht3x_start_periodic_measurement();
    if (error != NO_ERROR) {
        printf("SHT31 Start Measurement Error: %d\r\n", error);
        return error;
    }

    // 5. 打印初始化成功日志（对标SCD41的日志风格）
    printf("SHT31 Init Success !\r\n");
    return NO_ERROR;
}

/**
 * @brief 读取SHT31的温度和湿度数据
 * @note 参考SCD41_Read_CO2实现风格，先检查数据就绪（简化版），再读取数据
 * @return 0-读取成功，非0-错误码（注：原SCD41返回CO2值，这里返回错误码更合理，数据存在全局变量中）
 */
uint16_t SHT31_Read_Temp_Hum(void) {
    int16_t error = NO_ERROR;
    bool data_ready = true; // SHT31周期模式下数据持续更新，简化为直接读取

    // 1. 模拟数据就绪检查（SHT31无专门的data ready寄存器，简化处理）
    // 如需严格检查，可读取状态寄存器判断测量完成位
    if (data_ready) {
        // 2. 读取转换后的物理单位数据（毫摄氏度、毫百分比湿度）
        error = sht3x_read_measurement(&g_temp_m_deg_c, &g_hum_m_percent_rh);
        
        if (error == NO_ERROR) {
            // // 3. 打印数据（可选，对标SCD41的输出风格）
            // printf("SHT31 Temp: %.2f °C, Hum: %.2f %%RH\r\n",
            //        (float)g_temp_m_deg_c / 1000.0f,
            //        (float)g_hum_m_percent_rh / 1000.0f);
        } else {
            printf("SHT31 Read Data Error: %d\r\n", error);
        }
    }

    // 返回错误码（如需返回具体值，可修改为返回组合值或拆分函数）
    return (uint16_t)error;
}

// 单独读取温度（毫摄氏度）
int32_t SHT31_Read_Temperature(void) {
    return g_temp_m_deg_c;
}

// 单独读取湿度（毫百分比湿度）
int32_t SHT31_Read_Humidity(void) {
    return g_hum_m_percent_rh;
}