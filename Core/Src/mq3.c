/**
 * @file    mq3.c
 * @brief   MQ-3 酒精传感器 ADC 采样模块。
 * @details 配置 ADC1 通道，提供单次采样和多次平均采样结果。
 * @author  Microwave Oven
 */

#include "mq3.h"
#include "main.h"

extern ADC_HandleTypeDef hadc1; //ADC1 is used for MQ3 sensor

/**
 * @brief  读取指定ADC通道的单次转换值（HAL库版本）
 * @param  ch: ADC通道号（如ADC_CHANNEL_0 ~ ADC_CHANNEL_15，需和CubeMX配置一致）
 * @retval 转换结果（16位无符号整数，范围0~4095）
 */
uint16_t MQ3_Get_AlcoholConcentration(uint32_t ch)
{
    ADC_ChannelConfTypeDef sConfig = {0};

    // 1. 配置ADC规则通道（通道号、排名、采样时间）
    sConfig.Channel = ch;                  // 要采集的通道
    sConfig.Rank = 1;                      // 规则组排名1（仅单通道）
    sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES; // 采样时间239.5个周期（和原代码一致）
    if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
    {
        Error_Handler(); // 通道配置失败，进入错误处理
    }

    // 2. 启动ADC软件触发转换
    HAL_ADC_Start(&hadc1);

    // 3. 等待转换完成（超时时间10ms，防止卡死）
    if (HAL_ADC_PollForConversion(&hadc1, 10) != HAL_OK)
    {
        Error_Handler(); // 转换超时，进入错误处理
    }

    // 4. 返回转换结果（注意：HAL库返回32位，取低16位即可）
    return (uint16_t)HAL_ADC_GetValue(&hadc1);
}


/**
 * @brief  读取指定ADC通道的均值滤波结果（HAL库版本）
 * @param  ch: ADC通道号
 * @param  times: 平均次数
 * @retval 均值滤波后的转换结果
 */
uint16_t MQ3_Get_AlcoholConcentration_Average(uint32_t ch, uint8_t times)
{
    uint32_t temp_val = 0;
    uint8_t t;

    // 多次采集并累加
    for(t = 0; t < times; t++)
    {
        temp_val += MQ3_Get_AlcoholConcentration(ch);
        HAL_Delay(5); // HAL库延时5ms（替代原Delay_ms）
    }

    // 返回平均值
    return (uint16_t)(temp_val / times);
}

/* 错误处理函数（若未定义，需补充） */
#ifdef USE_FULL_ASSERT
void Error_Handler(void)
{
    /* 用户可添加自定义错误处理逻辑，如LED闪烁 */
    __disable_irq();
    while (1)
    {
        HAL_GPIO_ToogglePin(GPIOB, GPIO_PIN_13); // 假设PB13连接了LED
        HAL_Delay(300); // 300ms闪烁一次
    }
}
#endif
