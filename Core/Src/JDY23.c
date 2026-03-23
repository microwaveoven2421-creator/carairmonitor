#include "jdy23.h"
#include <string.h>

// 静态变量保存USART6句柄
static UART_HandleTypeDef *ble_uart6 = NULL;

void JDY23_Init(UART_HandleTypeDef *huart) {
    if (huart != NULL) {
        ble_uart6 = huart;
    }
}

void JDY23_SendString(const char *str) {
    // 空指针/空字符串保护，避免卡死
    if (ble_uart6 == NULL || str == NULL || strlen(str) == 0) {
        return;
    }
    // 轮询发送：USART6 + 500ms超时（足够发完传感器数据）
    HAL_UART_Transmit(ble_uart6, (uint8_t*)str, strlen(str), 500);
}