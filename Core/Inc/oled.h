#ifndef __OLED_H
#define __OLED_H

#include "i2c.h"

void OLED_Clear(void);

/**
 * @brief   向OLED驱动IC写入命令
 * 
 * @param   cmd 命令（一个字节）
 */
void OLED_WriteCommand(uint8_t cmd);

void OLED_SetCursor(uint8_t Y, uint8_t X);

/**
 * @brief   向OLED驱动IC写入数据,点亮某部分的像素（灯珠）
 * 
 * @param   data 数据(一个字节)
 */
void OLED_WriteData(uint8_t data);

void OLED_Init(void);

void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char);
void OLED_ShowString(uint8_t Line, uint8_t Column, char *String);
void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length);
void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);
void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length);

#endif // !__OLED_H