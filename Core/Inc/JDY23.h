#ifndef __JDY23_H
#define __JDY23_H

#include "stm32f4xx_hal.h"

// 初始化蓝牙（绑定USART6）
void JDY23_Init(UART_HandleTypeDef *huart);
// 发送字符串到蓝牙（轮询模式，无中断）
void JDY23_SendString(const char *str);

#endif /* __JDY23_H */