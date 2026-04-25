/**
 * @file    sht3x_i2c.c
 * @brief   SHT3x 低层 I2C 命令实现。
 * @details 负责发送测量命令、读取原始数据帧、校验 CRC 并转换物理单位。
 * @author  Microwave Oven
 */

#include "sht3x_i2c.h"
#include "sensirion_common.h"
#include "sensirion_i2c.h"
#include "sensirion_i2c_hal.h"

#define sensirion_hal_sleep_us sensirion_i2c_hal_sleep_usec

#define NO_ERROR 0
#define I2C_TRANSMIT_ERROR -1
#define I2C_RECEIVE_ERROR -2
#define CRC_CHECK_ERROR -3

static uint8_t communication_buffer[64]; // 用于存储I2C通信数据的缓冲区
static uint8_t _i2c_address = SHT3X_I2C_ADDR_44; // 默认I2C地址

void sht3x_init(uint8_t i2c_addr){
    _i2c_address = i2c_addr;
}

uint16_t sht3x_start_periodic_measurement(){
    int16_t local_error = NO_ERROR;
    uint8_t* buffer_ptr = communication_buffer;
    uint16_t local_offset = 0;
    
    // 添加SHT3X周期测量命令（每秒0.5次，Medium Repeatability），降低传感器自热
    // 对应枚举：SHT3X_MEASURE_PERIODIC_0_5MPS_MEDREP_CMD_ID = 0x2024
    local_offset = 
        sensirion_i2c_add_command16_to_buffer(buffer_ptr, local_offset, 
                                                    SHT3X_MEASURE_PERIODIC_0_5MPS_MEDREP_CMD_ID);
    
    // 硬件I2C发送数据（地址左移1位，因为HAL库需要7位地址+读写位）
    if (HAL_I2C_Master_Transmit(&SHT3X_I2C_HANDLE, (_i2c_address << 1), 
                               buffer_ptr, local_offset, 100) != HAL_OK) {
        local_error = I2C_TRANSMIT_ERROR;
        return local_error;
    }
    
    return local_error;
}

uint16_t sht3x_stop_periodic_measurement(){
    int16_t local_error = NO_ERROR;
    uint8_t* buffer_ptr = communication_buffer;
    uint16_t local_offset = 0;
    
    // 添加SHT3X停止周期测量命令
    // 对应枚举：SHT3X_STOP_PERIODIC_MEASUREMENT_CMD_ID = 0x3093
    local_offset = 
        sensirion_i2c_add_command16_to_buffer(buffer_ptr, local_offset, 
                                                   SHT3X_STOP_PERIODIC_MEASUREMENT_CMD_ID);
    
    // 硬件I2C发送数据（地址左移1位，因为HAL库需要7位地址+读写位）
    if (HAL_I2C_Master_Transmit(&SHT3X_I2C_HANDLE, (_i2c_address << 1), 
                               buffer_ptr, local_offset, 100) != HAL_OK) {
        local_error = I2C_TRANSMIT_ERROR;
        return local_error;
    }
    sensirion_i2c_hal_sleep_usec(500 * 1000); // 停止周期测量后需要等待至少500ms才能发送下一条命令
    return local_error;
}

int16_t sht3x_measure_single_shot() {
    int16_t local_error = NO_ERROR;
    uint8_t* buffer_ptr = communication_buffer;
    uint16_t local_offset = 0;
    
    // 发送单次测量命令（中等精度，无时钟延伸）
    local_offset = sensirion_i2c_add_command16_to_buffer(buffer_ptr, local_offset,
                                                    SHT3X_MEASURE_SINGLE_SHOT_MEDREP_CMD_ID);
    
    local_error = HAL_I2C_Master_Transmit(&SHT3X_I2C_HANDLE, (_i2c_address << 1),
                                         buffer_ptr, local_offset, 100);
    if (local_error != HAL_OK) {
        return I2C_TRANSMIT_ERROR;
    }
    
    HAL_Delay(15); // 单次测量响应时间（中等精度15ms）
    return NO_ERROR;
}

static uint8_t sht3x_crc_calculate(uint16_t data) {
    uint8_t crc = 0xFF;
    uint8_t byte;
    
    // 校验高字节
    byte = (uint8_t)(data >> 8);
    crc ^= byte;
    for (uint8_t i = 0; i < 8; i++) {
        crc = (crc << 1) ^ ((crc & 0x80) ? 0x31 : 0);
    }
    
    // 校验低字节
    byte = (uint8_t)(data & 0xFF);
    crc ^= byte;
    for (uint8_t i = 0; i < 8; i++) {
        crc = (crc << 1) ^ ((crc & 0x80) ? 0x31 : 0);
    }
    
    return crc;
}

int16_t sht3x_read_measurement_raw(uint16_t* temperature, uint16_t* humidity){
    int16_t local_error = NO_ERROR;
    uint8_t* buffer_ptr = communication_buffer;
    uint16_t local_offset = 0;
    
    // 1. 发送读取周期测量数据命令（0xE000）
    local_offset = sensirion_i2c_add_command16_to_buffer(buffer_ptr, local_offset,
                                                    SHT3X_FETCH_PERIODIC_MEASUREMENT_DATA_CMD_ID);
    
    local_error = HAL_I2C_Master_Transmit(&SHT3X_I2C_HANDLE, (_i2c_address << 1),
                                         buffer_ptr, local_offset, 100);
    if (local_error != HAL_OK) {
        return I2C_TRANSMIT_ERROR;
    }
    
    // 2. 短暂延时，确保传感器准备好数据（SHT3X响应时间<1ms）
    HAL_Delay(1);
    
    // 3. 读取6字节数据：温度高8位+低8位+CRC + 湿度高8位+低8位+CRC
    local_error = HAL_I2C_Master_Receive(&SHT3X_I2C_HANDLE, (_i2c_address << 1) | 0x01,
                                        buffer_ptr, 6, 100);
    if (local_error != HAL_OK) {
        return I2C_RECEIVE_ERROR;
    }
    
    // 4. 解析原始数据
    *temperature = sensirion_common_bytes_to_uint16_t(&buffer_ptr[0]);
    *humidity = sensirion_common_bytes_to_uint16_t(&buffer_ptr[3]);
    
    // 5. CRC校验
    uint8_t temp_crc = buffer_ptr[2];
    uint8_t hum_crc = buffer_ptr[5];
    uint8_t calc_temp_crc = sht3x_crc_calculate(*temperature);
    uint8_t calc_hum_crc = sht3x_crc_calculate(*humidity);
    
    if ((temp_crc != calc_temp_crc) || (hum_crc != calc_hum_crc)) {
        return CRC_CHECK_ERROR;
    }
    
    return NO_ERROR;
}


int16_t sht3x_read_measurement(int32_t* temperature_m_deg_c, int32_t* humidity_m_percent_rh){
    int16_t error;
    uint16_t raw_temperature;
    uint16_t raw_humidity;
    
    // 1. 读取原始数据
    error = sht3x_read_measurement_raw(&raw_temperature, &raw_humidity);
    if (error != NO_ERROR) {
        return error;
    }
    
    // 2. 转换为毫单位物理量（参考SCD4X的整数运算方式，避免浮点误差）
    // 温度公式：T(°C) = (175 * raw / 65535) - 45 → 毫摄氏度 = [(175 * raw * 1000) / 65535] - 45000
    *temperature_m_deg_c = ((175000LL * raw_temperature) / 65535) - 45000;
    
    // 湿度公式：RH(%) = (100 * raw / 65535) → 毫百分比 = (100000 * raw) / 65535
    *humidity_m_percent_rh = ((100000LL * raw_humidity) / 65535);
    
    return NO_ERROR;
}

int16_t sht3x_soft_reset(){
    int16_t local_error = NO_ERROR;
    uint8_t* buffer_ptr = communication_buffer;
    uint16_t local_offset = 0;

    // 1. 将SHT3X软件复位命令（0x30A2）添加到发送缓冲区
    // 对应枚举：SHT3X_SOFT_RESET_CMD_ID = 0x30A2
    local_offset = sensirion_i2c_add_command16_to_buffer(buffer_ptr, local_offset,
                                                    SHT3X_SOFT_RESET_CMD_ID);

    // 2. 通过硬件I2C发送复位命令
    // 注意：I2C地址左移1位（HAL库要求7位地址+读写位，写位为0）
    if (HAL_I2C_Master_Transmit(&SHT3X_I2C_HANDLE, (_i2c_address << 1),
                               buffer_ptr, local_offset, 100) != HAL_OK) {
        local_error = I2C_TRANSMIT_ERROR;
        return local_error;
    }

    // 3. 复位后延时（SHT3X复位完成需要至少1ms，推荐30ms确保稳定）
    // 30ms延时，保证传感器完全复位
    HAL_Delay(30);

    return local_error;
}

int16_t sht3x_general_call_reset() {
    int16_t local_error = NO_ERROR;
    uint8_t* buffer_ptr = communication_buffer;
    uint16_t local_offset = 0;

    // 广播复位命令（0x0006）
    local_offset = sensirion_i2c_add_command16_to_buffer(buffer_ptr, local_offset,
                                                    SHT3X_GENERAL_CALL_CMD_ID);

    // 广播地址为0x00，左移1位后为0x00
    if (HAL_I2C_Master_Transmit(&SHT3X_I2C_HANDLE, 0x00,
                               buffer_ptr, local_offset, 100) != HAL_OK) {
        local_error = I2C_TRANSMIT_ERROR;
        return local_error;
    }

    HAL_Delay(30);
    return local_error;
}
