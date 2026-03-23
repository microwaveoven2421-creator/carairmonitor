/**
 * @brief  PMS5003激光传感器驱动（对标SHT31代码风格）
 * @note   主动式传输协议，波特率：9600bps，校验位：无，停止位：1位
 *         使用USART2--PA2-TX(接模块引脚4)、PA3-RX(接模块引脚5)
 *         需开启串口2接收中断回调函数处理数据
 * @author Microwave Oven
 * @date   2026.03.05 v3.0
 *         2026.01.17 v2.0 
 *         2026.01.16 v1.0
 * 
 * 重构优化点：
 * 1. 统一命名风格（驼峰式），对标SHT31/SCD41
 * 2. 新增错误码定义，完善错误处理
 * 3. 模块化拆分函数，增强可读性
 * 4. 补充详细注释，统一文档风格
 * 5. 保留原有核心功能，仅优化代码结构
 */
#include "PMS5003.h"
#include "usart.h"
#include "main.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

/* ====================== 宏定义 ====================== */
#define NO_ERROR               0       // 无错误
#define PMS5003_FRAME_LEN      32      // 一帧数据总长度
#define PMS5003_DATA_LEN       30      // 除起始符外的数据长度
#define PMS5003_START_BYTE1    0x42    // 起始符1
#define PMS5003_START_BYTE2    0x4D    // 起始符2
#define PMS5003_UART_TIMEOUT   HAL_MAX_DELAY // 串口发送超时时间

/* ============= 全局变量（数据缓存，对标SHT31的全局温湿度） ============ */
uint16_t g_PM1_0_Value = 0;//大气环境PM1.0的值
uint16_t g_PM2_5_Value = 0;//大气环境PM2.5的值
uint16_t g_PM10_0_Value = 0;//大气环境PM10.0的值

/* ====================== 静态变量（内部使用） ====================== */
typedef enum {
    PMS5003_STATE_IDLE = 0,            // 空闲状态
    PMS5003_STATE_MATCH_START1,        // 匹配到起始符1
    PMS5003_STATE_RECEIVE_DATA,        // 接收数据中
    PMS5003_STATE_DATA_COMPLETE        // 数据接收完成
} PMS5003_StateTypeDef;

static PMS5003_StateTypeDef s_state = PMS5003_STATE_IDLE; // 状态机
static uint8_t s_data_cnt = 0;                          // 数据接收计数
static uint8_t s_rx_Buffer[PMS5003_FRAME_LEN] = {0};    // 接收缓冲区

/* ====================== 静态函数声明（内部使用） ====================== */
static int16_t PMS5003_CheckSum(uint8_t* data_Buffer);
static void PMS5003_AnalyseData(uint8_t* data_Buffer);
static bool is_first_run = true; // 首次运行标志（用于判断是否有有效数据）

/**
 * @brief  PMS5003数据接收状态机（串口中断回调中调用）
 * @note   串口每接收1字节调用此函数，完成32字节数据组帧
 * @param  data: 串口接收到的单字节数据
 * @retval int16_t: 0-正常，非0-错误码（此处仅返回状态，无错误）
 */
int16_t PMS5003_ReceiveData(uint8_t data) {
    switch(s_state) {
        case PMS5003_STATE_IDLE: // 空闲状态，匹配起始符1
            if(data == PMS5003_START_BYTE1) {
                s_state = PMS5003_STATE_MATCH_START1;
                s_rx_Buffer[0] = data;
            }
            break;

        case PMS5003_STATE_MATCH_START1: // 匹配到起始符1，匹配起始符2
            if(data == PMS5003_START_BYTE2) {
                s_state = PMS5003_STATE_RECEIVE_DATA;
                s_rx_Buffer[1] = data;
                s_data_cnt = 0; // 计数清零
            } else {
                s_state = PMS5003_STATE_IDLE; // 匹配失败，回到空闲态
            }
            break;

        case PMS5003_STATE_RECEIVE_DATA: // 接收剩余30字节数据
            s_rx_Buffer[2 + s_data_cnt++] = data;
            if(s_data_cnt >= PMS5003_DATA_LEN) { // 数据接收完成
                s_state = PMS5003_STATE_DATA_COMPLETE;
            }
            break;

        case PMS5003_STATE_DATA_COMPLETE: // 一帧数据接收完成
            PMS5003_AnalyseData(s_rx_Buffer); // 解析数据
            // 重置状态机，等待下一帧数据
            s_state = PMS5003_STATE_IDLE;
            s_data_cnt = 0;
            memset(s_rx_Buffer, 0, PMS5003_FRAME_LEN);
            break;

        default:
            s_state = PMS5003_STATE_IDLE;
            break;
    }
    return NO_ERROR;
}



/**
 * @brief  PMS5003校验和计算
 * @note   校验码 = 起始符1 + 起始符2 + ... + 数据13低八位
 * @param  data_Buffer: 完整的32字节数据缓冲区
 * @retval int16_t: 0-校验通过，-1-校验失败
 */
static int16_t PMS5003_CheckSum(uint8_t* data_Buffer) {
    uint16_t sum = 0;
    uint16_t check_sum = 0;

    // 计算前30字节的和
    for(uint8_t i = 0; i < PMS5003_DATA_LEN; i++) {
        sum += data_Buffer[i];
    }

    // 提取协议中的校验位（第30、31字节）
    check_sum = (data_Buffer[30] << 8) | data_Buffer[31];

    // 校验和判断
    if(sum == check_sum) {
        return NO_ERROR;
    } else {
        return -1; // 校验失败错误码
    }
}

/**
 * @brief  PMS5003数据解析（校验通过后更新全局缓存）
 * @note   解析完成后，数据存入全局变量g_PM1_0_Value/g_PM2_5_Value/g_PM10_0_Value
 * @param  data_Buffer: 完整的32字节数据缓冲区
 * @retval None
 */
static void PMS5003_AnalyseData(uint8_t* data_Buffer) {
    int16_t check_err = NO_ERROR;

    // 1. 校验和检查
    check_err = PMS5003_CheckSum(data_Buffer);
    if(check_err != NO_ERROR) {
        printf("PMS5003 CheckSum Error!\r\n");
        return; // 校验失败，不更新数据
    }

    // 2. 解析PM值（大气环境）
    g_PM1_0_Value = (data_Buffer[10] << 8) | data_Buffer[11];
    g_PM2_5_Value = (data_Buffer[12] << 8) | data_Buffer[13];
    g_PM10_0_Value = (data_Buffer[14] << 8) | data_Buffer[15];

    // // 3. 调试打印（可选）
    // printf("PMS5003 Parse Success - PM1.0:%d | PM2.5:%d | PM10:%d μg/m³\r\n",
    //        g_PM1_0_Value, g_PM2_5_Value, g_PM10_0_Value);
}

/**
 * @brief  PMS5003数据读取函数（对标SHT31_Read_Temp_Hum）
 * @note   读取缓存的PM值，返回错误码，数据存在全局变量中
 * @retval int16_t: 0-读取成功，-1-无有效数据
 */
int16_t PMS5003_Read_PM_Value(void) {
    // 1. 首次运行时：不判断无有效数据，直接返回成功
    if (is_first_run) {
        is_first_run = false; // 首次运行后，标记为非首次
        return PMS5003_ERR_NONE;
    }
     // 2. 非首次运行时：正常判断是否有有效数据
    if (g_PM2_5_Value == 0) {
        printf("PMS5003 No Valid Data!\r\n");
        return PMS5003_ERR_NO_DATA;
    }
    return PMS5003_ERR_NONE;
}

/**
 * @brief  PMS5003数据格式化发送（对标SHT31的打印逻辑）
 * @note   根据指定类型发送PM1.0/2.5/10数据，兼容原有read_pms5003函数
 * @param  huart: 串口句柄（如&huart1）
 * @param  pData: 发送缓冲区
 * @param  pm_type: 1-PM1.0，2-PM2.5，3-PM10
 * @retval int16_t: 0-发送成功，非0-错误码
 */
int16_t PMS5003_Send_PM_Value(UART_HandleTypeDef* huart, uint8_t *pData, uint8_t pm_type) {
    int16_t error = NO_ERROR;

    switch(pm_type) {
        case 1: // PM1.0
            sprintf((char*)pData, "PM1.0: %d ug/m3\r\n", g_PM1_0_Value);
            break;

        case 2: // PM2.5
            sprintf((char*)pData, "PM2.5: %d ug/m3\r\n", g_PM2_5_Value);
            break;

        case 3: // PM10
            sprintf((char*)pData, "PM10: %d ug/m3\r\n", g_PM10_0_Value);
            break;

        default: // 参数错误
            sprintf((char*)pData, "PMS5003 Param Error!\r\n");
            error = -2; // 参数错误码
            break;
    }

    // 串口发送数据
    if(HAL_UART_Transmit(huart, pData, strlen((char*)pData), PMS5003_UART_TIMEOUT) != HAL_OK) {
        error = -3; // 串口发送错误码
        printf("PMS5003 UART Transmit Error!\r\n");
    }

    HAL_Delay(50); // 发送间隔
    return error;
}

/**
 * @brief  兼容原有接口的读取函数（无需修改main.c调用逻辑）
 * @param  huart: 串口句柄
 * @param  pData: 发送缓冲区
 * @param  pm_type: 1-PM1.0，2-PM2.5，3-PM10
 * @retval None
 */
void read_pms5003(UART_HandleTypeDef* huart, uint8_t *pData, uint8_t pm_type) {
    PMS5003_Send_PM_Value(huart, pData, pm_type);
}

/**
 * @brief  PMS5003初始化函数（对标SHT31_Init）
 * @note   初始化状态机和全局变量，开启串口中断
 * @retval int16_t: 0-成功，非0-错误码
 */
int16_t PMS5003_Init(void) {
    // 初始化状态机和缓冲区
    s_state = PMS5003_STATE_IDLE;
    s_data_cnt = 0;
    memset(s_rx_Buffer, 0, PMS5003_FRAME_LEN);

    // 初始化全局PM值
    g_PM1_0_Value = 0;
    g_PM2_5_Value = 0;
    g_PM10_0_Value = 0;

    //重置首次运行标志位
    is_first_run = true; 

    // 打印初始化成功日志
    printf("PMS5003 Init Success!\r\n");
    return NO_ERROR;
}