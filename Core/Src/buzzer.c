#include "buzzer.h"
#include "tim.h"

/* 静态函数声明（内部使用） */
static void Buzzer_Set_Freq(uint32_t freq);
static void Buzzer_Set_Duty(uint8_t duty);
static void Buzzer_On(void);
static void Buzzer_Off(void);

/**
 * @brief  设置蜂鸣器频率（通过修改定时器ARR值）
 * @param  freq: 目标频率（Hz）
 * @retval None
 */
static void Buzzer_Set_Freq(uint32_t freq) {
    TIM_HandleTypeDef *htim = BUZZER_TIM_HANDLE;
    uint32_t tim_clk = HAL_RCC_GetPCLK2Freq() * 2; // TIM3挂在APB1，PCLK1=42MHz→TIM时钟=84MHz（需根据实际调整）
    uint32_t psc = (tim_clk / 1000000) - 1;        // 分频到1MHz（便于计算）
    uint32_t arr = (1000000 / freq) - 1;           // ARR = 1MHz/频率 - 1

    // 配置定时器预分频器和自动重装值
    __HAL_TIM_SET_PRESCALER(htim, psc);
    __HAL_TIM_SET_AUTORELOAD(htim, arr);
    HAL_TIM_Base_Start(htim); // 重启定时器使配置生效
}

/**
 * @brief  设置蜂鸣器占空比
 * @param  duty: 占空比（0~100）
 * @retval None
 */
static void Buzzer_Set_Duty(uint8_t duty) {
    TIM_HandleTypeDef *htim = BUZZER_TIM_HANDLE;
    uint32_t arr = __HAL_TIM_GET_AUTORELOAD(htim);
    uint32_t ccr = (arr + 1) * duty / 100; // 计算比较值

    __HAL_TIM_SET_COMPARE(htim, BUZZER_TIM_CHANNEL, ccr);
}

/**
 * @brief  蜂鸣器开启（输出PWM）
 * @retval None
 */
static void Buzzer_On(void) {
    HAL_TIM_PWM_Start(BUZZER_TIM_HANDLE, BUZZER_TIM_CHANNEL);
}

/**
 * @brief  蜂鸣器关闭（停止PWM）
 * @retval None
 */
static void Buzzer_Off(void) {
    HAL_TIM_PWM_Stop(BUZZER_TIM_HANDLE, BUZZER_TIM_CHANNEL);
}

/**
 * @brief  蜂鸣器初始化
 * @param  freq: 频率（Hz），0则用默认值
 * @retval None
 */
void Buzzer_Init(uint32_t freq) {
    // 初始化频率（默认1kHz）
    if (freq == 0) freq = BUZZER_DEFAULT_FREQ;
    Buzzer_Set_Freq(freq);

    // 初始化占空比（默认50%）
    Buzzer_Set_Duty(BUZZER_DEFAULT_DUTY);

    // 初始状态：关闭
    Buzzer_Off();
}

/**
 * @brief  通用报警函数
 * @param  duration_ms: 单次响的时长
 * @param  interval_ms: 循环间隔（停的时长）
 * @param  mode: 报警模式
 * @retval None
 */
void Buzzer_Alarm(uint32_t duration_ms, uint32_t interval_ms, Buzzer_Alarm_Mode mode) {
    switch (mode) {
        case BUZZER_ALARM_ONCE:
            Buzzer_On();
            HAL_Delay(duration_ms);
            Buzzer_Off();
            break;

        case BUZZER_ALARM_LOOP:
            Buzzer_On();
            HAL_Delay(duration_ms);
            Buzzer_Off();
            HAL_Delay(interval_ms);
            break;

        case BUZZER_ALARM_BEEP:
            Buzzer_On();
            HAL_Delay(100); // 固定短鸣100ms
            Buzzer_Off();
            break;

        default:
            break;
    }
}

/**
 * @brief  传感器阈值报警（一键调用，适配所有传感器）
 * @param  sensor_value: 传感器当前值
 * @param  threshold: 报警阈值
 * @param  duration_ms: 报警时长
 * @retval None
 */
void Buzzer_Sensor_Alarm(uint32_t sensor_value, uint32_t threshold, uint32_t duration_ms) {
    static uint8_t alarm_flag = 0; // 防重复报警标志

    if (sensor_value > threshold) {
        if (alarm_flag == 0) {
            Buzzer_Alarm(duration_ms, 0, BUZZER_ALARM_ONCE); // 单次报警
            alarm_flag = 1;
        }
    } else {
        alarm_flag = 0; // 阈值以下，清标志
        Buzzer_Stop();
    }
}

/**
 * @brief  停止蜂鸣器
 * @retval None
 */
void Buzzer_Stop(void) {
    Buzzer_Off();
}