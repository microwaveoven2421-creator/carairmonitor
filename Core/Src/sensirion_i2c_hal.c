/*
 * Copyright (c) 2018, Sensirion AG
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * * Neither the name of Sensirion AG nor the names of its
 *   contributors may be used to endorse or promote products derived from
 *   this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "sensirion_i2c_hal.h"
#include "sensirion_common.h"
#include "sensirion_config.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>

extern I2C_HandleTypeDef hi2c1;

//全局变量，标记DWT是否已经初始化（用于微秒休眠）
static uint8_t _dwt_initialized = 0;

/*
 * INSTRUCTIONS
 * ============
 *
 * Implement all functions where they are marked as IMPLEMENT.
 * Follow the function specification in the comments.
 */

/**
 * Select the current i2c bus by index.
 * All following i2c operations will be directed at that bus.
 *
 * THE IMPLEMENTATION IS OPTIONAL ON SINGLE-BUS SETUPS (all sensors on the same
 * bus)
 *
 * @param bus_idx   Bus index to select
 * @returns         0 on success, an error code otherwise
 */
int16_t sensirion_i2c_hal_select_bus(uint8_t bus_idx) {
    /* TODO:IMPLEMENT or leave empty if all sensors are located on one single
     * bus
     */
    (void)bus_idx; // avoid unused parameter warning
    return NO_ERROR;
}

/**
 * Initialize all hard- and software components that are needed for the I2C
 * communication.
 */
void sensirion_i2c_hal_init(void) {
    /* TODO:IMPLEMENT */
    //初始化DWT（Data Watchpoint and Trace unit）以实现微秒级的休眠
    if (!_dwt_initialized) {
        CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; //使能DWT模块
        DWT->CYCCNT = 0; //重置周期计数器
        DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk; //使能周期计数器
        _dwt_initialized = 1;
    }
    // 兜底检查I2C是否就绪
    if (hi2c1.State != HAL_I2C_STATE_READY) {
        HAL_I2C_Init(&hi2c1);
    }
}

/**
 * Release all resources initialized by sensirion_i2c_hal_init().
 */
void sensirion_i2c_hal_free(void) {
    /* TODO:IMPLEMENT or leave empty if no resources need to be freed */
}

/**
 * Execute one read transaction on the I2C bus, reading a given number of bytes.
 * If the device does not acknowledge the read command, an error shall be
 * returned.
 *
 * @param address 7-bit I2C address to read from
 * @param data    pointer to the buffer where the data is to be stored
 * @param count   number of bytes to read from I2C and store in the buffer
 * @returns 0 on success, error code otherwise
 */
/**
 * I2C读数据实现（封装HAL_I2C_Master_Receive）
 */
int8_t sensirion_i2c_hal_read(uint8_t address, uint8_t* data, uint8_t count) {
    /* TODO:IMPLEMENT */
    // 检查参数合法性
    if (data == NULL || count == 0) {
        return -1;
    }

    // HAL库要求地址左移1位 + 读位（0x01）
    uint16_t i2c_addr = (address << 1) | 0x01;
    HAL_StatusTypeDef hal_status = HAL_I2C_Master_Receive(&hi2c1, i2c_addr, data, count, 100);

    // 0为正常，-1为错误
    return (hal_status == HAL_OK) ? 0 : -1;
}

/**
 * Execute one write transaction on the I2C bus, sending a given number of
 * bytes. The bytes in the supplied buffer must be sent to the given address. If
 * the slave device does not acknowledge any of the bytes, an error shall be
 * returned.
 *
 * @param address 7-bit I2C address to write to
 * @param data    pointer to the buffer containing the data to write
 * @param count   number of bytes to read from the buffer and send over I2C
 * @returns 0 on success, error code otherwise
 */
/**
 * I2C写数据实现（封装HAL_I2C_Master_Transmit）
 */
int8_t sensirion_i2c_hal_write(uint8_t address, const uint8_t* data,
                               uint8_t count) {
    /* TODO:IMPLEMENT */
    // 检查参数合法性
    if (data == NULL || count == 0) {
        return -1;
    }

    // HAL库要求地址左移1位 + 写位（0x00）
    uint16_t i2c_addr = (address << 1) & 0xFE;
    HAL_StatusTypeDef hal_status = HAL_I2C_Master_Transmit(&hi2c1, i2c_addr, (uint8_t*)data, count, 100);

    // 0为正常，-1为错误
    return (hal_status == HAL_OK) ? 0 : -1;
}

/**
 * Sleep for a given number of microseconds. The function should delay the
 * execution for at least the given time, but may also sleep longer.
 *
 * Despite the unit, a <10 millisecond precision is sufficient.
 *
 * @param useconds the sleep time in microseconds
 */
/**
 * 微秒级休眠实现（基于DWT计数器，适配168MHz主频）
 */
void sensirion_i2c_hal_sleep_usec(uint32_t useconds) {
    /* TODO:IMPLEMENT */
    // 若DWT未初始化，先初始化
    if (!_dwt_initialized) {
        sensirion_i2c_hal_init();
    }

    // 短延时（<10us）直接用空循环兜底（DWT可能有误差）
    if (useconds < 10) {
        for (volatile uint32_t i = 0; i < useconds * 168; i++);
        return;
    }

    // 计算需要的时钟周期数：168MHz = 168个周期/1us
    uint32_t ticks = (SystemCoreClock / 1000000) * useconds;
    uint32_t start_tick = DWT->CYCCNT;

    // 等待计数器达到目标值（处理计数器溢出）
    while ((DWT->CYCCNT - start_tick) < ticks) {
        __NOP(); // 空操作，避免编译器优化
    }
}

