#include "sensor_manager.h"
#include "sht31.h"
#include "PMS5003.h"
#include "scd41.h"
#include "scd4x_i2c.h"
#include "mq3.h"
#include "buzzer.h"
#include "oled.h"
#include "jdy23.h"
#include "sd_spi.h"
#include "sdcard_app.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h> 

#define MQ3_BASE_ADC            360    // 无酒精时的基准ADC值（可根据实际零点调整）
#define MQ3_CALIB_COEF          0.02   // 校准系数（适配12位ADC）
#define MQ3_ALARM_MGL           1.0f   // 酒精报警阈值(mg/L)
#define NO_ERROR                0       // 无错误

/* ====================== 全局变量定义 ====================== */
int32_t g_temp_m_deg_c = 0;        // 全局温度值（毫摄氏度）
int32_t g_hum_m_percent_rh = 0;    // 全局湿度值（毫百分比湿度）

extern uint8_t UART2_RxData; // PMS5003串口接收数据变量（在main.c中定义）
extern UART_HandleTypeDef huart6; // JDY23使用的串口句柄（在main.c中定义）
/* ====================== 静态函数声明（内部使用） ====================== */
static void SHT31_Data_Convert(int32_t temp_m_deg_c, int32_t hum_m_percent_rh, 
                               float* temp, float* hum);

// ========== 适配HSE 8MHz的精准计时（无RTC） ==========
static uint32_t sys_total_seconds = 0;  // 系统运行总秒数
static uint32_t last_tick_ms = 0;       // 上一次计时的毫秒数

// 获取精准运行时间戳
void Get_System_Time_Stamp(char *time_buf, uint8_t buf_len) {
    if (time_buf == NULL || buf_len < 20) return;

    // 基于HAL_GetTick（1ms中断，HSE 8MHz配置下精准）
    uint32_t current_tick = HAL_GetTick();
    // 每1000ms更新一次秒数
    if (current_tick - last_tick_ms >= 1000) {
        sys_total_seconds += (current_tick - last_tick_ms) / 1000;
        last_tick_ms = current_tick;
    }

    // 计算时/分/秒（格式：RunTime: 00:05:36）
    uint32_t hours = sys_total_seconds / 3600;
    uint32_t minutes = (sys_total_seconds % 3600) / 60;
    uint32_t seconds = sys_total_seconds % 60;

    // 时间戳格式化（适配你的对齐宽度）
    sprintf(time_buf, "%02"PRIu32":%02"PRIu32":%02"PRIu32"", hours, minutes, seconds);
}

// ========== MQ3 ADC值转酒精浓度（mg/L） ==========
// 转换ADC值为酒精浓度（mg/L），并做异常值过滤
float MQ3_Convert_To_MgL(uint16_t adc_val) {
    // 异常值过滤：ADC值低于基准值，视为0 mg/L
    if (adc_val <= MQ3_BASE_ADC) {
        return 0.0f;
    }
    // 超过ADC最大值（4095），视为最大值
    if (adc_val >= 4095) {
        adc_val = 4095;
    }
    // 标定公式计算真实浓度
    float alcohol_mgL = (adc_val - MQ3_BASE_ADC) * MQ3_CALIB_COEF;
    // 保留2位小数，符合显示精度
    return (float)((int)(alcohol_mgL * 100)) / 100;
}

/* ====================== OLED显示传感器数据 ====================== */
// 浮点数据显示封装（适配OLED驱动）
static void OLED_ShowFloat(uint8_t Line, uint8_t Column, float Num, uint8_t IntLen, uint8_t DecLen)
{
    uint32_t IntNum, DecNum;
    float DecFloat;
  
    IntNum = (uint32_t)Num;                // 提取整数部分
    DecFloat = Num - IntNum;               // 提取小数部分
    for(uint8_t i=0; i<DecLen; i++){
        DecFloat *= 10;
    }
    DecNum = (uint32_t)DecFloat;
  
    OLED_ShowNum(Line, Column, IntNum, IntLen);    // 显示整数
    OLED_ShowChar(Line, Column + IntLen, '.');     // 显示小数点
    OLED_ShowNum(Line, Column + IntLen + 1, DecNum, DecLen); // 显示小数
}

// 所有传感器数据显示到OLED（排版适配0.96寸OLED）
static void Sensor_Show_OLED(float temp, float hum, uint16_t pm25, float alcohol_mgL, uint16_t co2, 
                             uint8_t alarm_type) {
    OLED_Clear(); // 清屏避免残留
  
    /* 第0行：温度+湿度 */
    OLED_ShowString(0, 0, "T:");
    OLED_ShowFloat(0, 2, temp, 2, 2);    // 温度（2位整数+2位小数）
    OLED_ShowChar(0, 6, 'C');
  
    OLED_ShowString(0, 8, "H:");
    OLED_ShowFloat(0, 10, hum, 2, 2);   // 湿度（2位整数+2位小数）
    OLED_ShowChar(0, 14, '%');
    if(alarm_type & 0x01) OLED_ShowChar(0, 15, '!'); // 温度报警标记
    else if(alarm_type & 0x02) OLED_ShowChar(0, 15, '!'); // 湿度报警标记
  
    /* 第1行：PM2.5 */
    OLED_ShowString(1, 0, "PM2.5:");
    OLED_ShowNum(1, 7, pm25, 3);        // PM2.5（3位数字）
    OLED_ShowString(1, 10, "ug/m3");
    if(alarm_type & 0x08) OLED_ShowChar(1, 15, '!'); // PM2.5报警标记
  
    /* 第2行：酒精浓度 */
    OLED_ShowString(2, 0, "Alcohol:");
    OLED_ShowFloat(2, 9, alcohol_mgL, 1, 2); // 酒精浓度（1位整数+2位小数）
    OLED_ShowString(2, 14, "mg"); // 显示单位mg/L（L超出OLED列数，简化为mg）
    if(alarm_type & 0x04) OLED_ShowChar(2, 15, '!'); // 酒精报警标记
  
    /* 第3行：CO2 */
    OLED_ShowString(3, 0, "CO2:");
    OLED_ShowNum(3, 7, co2, 4);         // CO2值（4位数字）
    OLED_ShowString(3, 11, "ppm");      // 优化：直接显示ppm字符串
    if(alarm_type & 0x10) OLED_ShowChar(3, 15, '!'); // CO2报警标记
  
    // 显示报警提示（有报警时）
    // if(alarm_type != 0) {
    //     OLED_ShowString(0, 0, "ALARM!"); // 首行显示报警提醒
    // }
  
    HAL_Delay(10); // 短暂延时确保显示完成
}

/* ====================== 报警信息发送函数 ====================== */
static void Send_Alarm_Message(const char* alarm_source, const char* alarm_detail) {
    char alarm_buf[128] = {0};
    
    // 1. 串口打印报警信息
    printf("\r\n!!! %s !!!\r\nDetail: %s\r\n", alarm_source, alarm_detail);
    
    // 2. 蓝牙发送报警信息
    sprintf(alarm_buf, "\r\n=== ALARM ===\r\nSource: %s\r\nDetail: %s\r\n=============\r\n", 
            alarm_source, alarm_detail);
    JDY23_SendString(alarm_buf);
}

/* ====================== 传感器系统初始化 ====================== */
void Sensor_Manager_Init(void) {
    int16_t sht31_init_err = NO_ERROR;
    int16_t pms5003_init_err = NO_ERROR;
    int16_t scd41_init_err = NO_ERROR;

    // 0. 初始化OLED显示屏
    OLED_Init(); 
    OLED_ShowString(0, 1, "HELLO!");
    OLED_ShowString(1, 4, "USER!");
    HAL_Delay(1000); // 显示初始化成功提示1秒
    OLED_Clear();

    // 1. 初始化SHT31传感器
    sht31_init_err = SHT31_Init();
    if (sht31_init_err != NO_ERROR) {
        printf("SHT31 Init Failed! Error Code: %d\r\n", sht31_init_err);
        while (1); // 初始化失败卡死
    }
    
    // 2. 初始化PMS5003传感器
    pms5003_init_err = PMS5003_Init();
    if (pms5003_init_err != NO_ERROR) {
        printf("PMS5003 Init Failed! Error Code: %d\r\n", pms5003_init_err);
        while (1); // 初始化失败卡死
    }

    // 3. 初始化PMS5003串口接收中断
    HAL_UART_Receive_IT(&PMS5003_UART_HANDLE, &UART2_RxData, 1);
    
    // 4. SCD41初始化
    scd41_init_err = SCD41_Init();
    if (scd41_init_err != NO_ERROR) {
        printf("SCD41 Init Failed! Error Code: %d\r\n", scd41_init_err);
        while (1); // 初始化失败卡死
    }

    // 5. 蜂鸣器初始化
    Buzzer_Init(1000); // 设置初始频率为1kHz

    // 6. 初始化SD卡
    if (SD_Card_Init() == SD_CARD_OK) {
        printf("SD Card Init Success!\r\n");
    } else {
        printf("SD Card Init Failed! Offline logging disabled.\r\n");
    }

    // 7. 初始化USART6蓝牙
    JDY23_Init(&huart6);
    JDY23_SendString("\r\n HELLO USER!\r\r\n"); // 蓝牙发送启动提示

    // 8. 串口打印启动信息
    printf("====================================================\r\n");
    printf("        多传感器数据采集系统启动\r\n");
    printf("采集间隔 :%dms | 温度精度 :%.1f位| 湿度精度 :%.1f位\r\n",
            COLLECT_INTERVAL_MS, (float)TEMP_DISPLAY_PRECISION, (float)HUM_DISPLAY_PRECISION);
    printf("酒精阈值:%.1fmg/L | CO2阈值:%dppm | PM2.5阈值:%dμg/m³\r\n",
            MQ3_ALARM_MGL, CO2_ALARM_THRESHOLD, PM25_ALARM_THRESHOLD);
    printf("====================================================\r\n");
}

/* ====================== 温湿度数据转换为精确浮点数 ====================== */
static void SHT31_Data_Convert(int32_t temp_m_deg_c, int32_t hum_m_percent_rh, 
                               float* temp, float* hum) {
    *temp = (float)temp_m_deg_c / 1000.0f;
    *hum = (float)hum_m_percent_rh / 1000.0f;
}

/* ====================== 所有传感器数据采集+统一报警 ====================== */
void Sensor_Manager_Collect_Alarm(void) {
    // 1. 变量定义
    float temp = 0.0f, hum = 0.0f;
    uint16_t sht31_read_err = NO_ERROR;
    int16_t pms5003_read_err = NO_ERROR;
    
    uint16_t mq3_val = 0;        // 酒精ADC值
    uint16_t co2_val = 0;        // CO2值
    uint16_t pm25_val = 0;       // PM2.5值
    uint8_t alarm_type = 0;      // 报警类型标记：0x01-温度 0x02-湿度 0x04-酒精 0x08-PM2.5 0x10-CO2
    char alarm_detail[64] = {0}; // 报警详情

    // 1. 读取SCD41-co2数据
    co2_val = SCD41_Read_CO2();
    HAL_Delay(5); // 释放I2C1总线

    // 2. 采集SHT31温湿度数据（非I2C1，原有逻辑不变）
    sht31_read_err = SHT31_Read_Temp_Hum();
    if (sht31_read_err != NO_ERROR) {
        printf("SHT31 Read Data Error! Error Code: %d\r\n", sht31_read_err);
    } else {
        g_temp_m_deg_c = SHT31_Read_Temperature();
        g_hum_m_percent_rh = SHT31_Read_Humidity();
        SHT31_Data_Convert(g_temp_m_deg_c, g_hum_m_percent_rh, &temp, &hum);
        
        // 温度报警判断（低于低阈值 或 高于高阈值）
        if (temp < TEMP_ALARM_LOW || temp > TEMP_ALARM_HIGH) {
            alarm_type |= 0x01; // 标记温度报警
            sprintf(alarm_detail, "Temperature: %.2f°C (Normal: %.1f~%.1f°C)", 
                    temp, TEMP_ALARM_LOW, TEMP_ALARM_HIGH);
            Send_Alarm_Message("Temperature Alarm", alarm_detail);
        }
        
        // 湿度报警判断（低于低阈值 或 高于高阈值）
        if (hum < HUM_ALARM_LOW || hum > HUM_ALARM_HIGH) {
            alarm_type |= 0x02; // 标记湿度报警
            sprintf(alarm_detail, "Humidity: %.2f%% (Normal: %.1f~%.1f%%)", 
                    hum, HUM_ALARM_LOW, HUM_ALARM_HIGH);
            Send_Alarm_Message("Humidity Alarm", alarm_detail);
        }
    }

    // 3. 采集PMS5003 PM2.5数据
    pms5003_read_err = PMS5003_Read_PM_Value();
    if (pms5003_read_err != NO_ERROR) {
        printf("PMS5003 Read Data Error! Error Code: %d\r\n", pms5003_read_err);
    } else {
        pm25_val = g_PM2_5_Value;
        
        // PM2.5报警判断
        if (pm25_val > PM25_ALARM_THRESHOLD && pm25_val != 0) {
            alarm_type |= 0x08; // 标记PM2.5报警
            sprintf(alarm_detail, "PM2.5: %d ug/m3 (Threshold: %d ug/m3)", 
                    pm25_val, PM25_ALARM_THRESHOLD);
            Send_Alarm_Message("PM2.5 Alarm", alarm_detail);
        }
    }

    // 4. 采集MQ3酒精数据
    mq3_val = MQ3_Get_AlcoholConcentration_Average(ADC_CHANNEL_0, 10);
    float alcohol_mgL = MQ3_Convert_To_MgL(mq3_val); // 转换为mg/L浓度值
    
    // 酒精报警判断
    if (alcohol_mgL >= MQ3_ALARM_MGL) {
        alarm_type |= 0x04; // 标记酒精报警
        sprintf(alarm_detail, "Alcohol: %.2f mg/L (Threshold: %.1f mg/L)", 
                alcohol_mgL, MQ3_ALARM_MGL);
        Send_Alarm_Message("Alcohol Alarm", alarm_detail);
    }

    // 5. CO2报警判断
    if (co2_val > CO2_ALARM_THRESHOLD && co2_val != 0) {
        alarm_type |= 0x10; // 标记CO2报警
        sprintf(alarm_detail, "CO2: %d ppm (Threshold: %d ppm)", 
                co2_val, CO2_ALARM_THRESHOLD);
        Send_Alarm_Message("CO2 Alarm", alarm_detail);
    }

    // 6. 统一打印所有5个传感器数据
    printf("\r\n============= All Sensor Data ==============\r\n");
    if(pm25_val == 0) {// 如果pm2.5为0，显示"waiting"
        printf("Temp:%.2fC | Humidity:%.2f%% | PM2.5:waiting...\r\n", temp, hum);
    } else {
        printf("Temp:%.2fC | Humidity:%.2f%% | PM2.5: %d ug/m3\r\n", temp, hum, pm25_val);
    }
    if(co2_val == 0) {// 如果co2为0，显示"waiting"
        printf("Alcohol:%.2fmg/L | CO2:waiting...\r\n", alcohol_mgL);
    } else {
        printf("Alcohol:%.2fmg/L | CO2:%dppm \r\n", alcohol_mgL, co2_val);
    }
    printf("======================================\r\n");

    // 蓝牙发送数据 
    // 定义蓝牙发送缓冲区和时间戳缓冲区
    char ble_buf[256] = {0};
    char time_stamp[32] = {0};

    // 第一步：获取并发送时间戳
    Get_System_Time_Stamp(time_stamp, sizeof(time_stamp));

    /* ====================== SD卡离线记录 ====================== */
    if (SD_Card_IsReady()) {
        SD_SensorRecord_t record;

        memset(&record, 0, sizeof(record));
        strncpy(record.time_stamp, time_stamp, sizeof(record.time_stamp) - 1);

        record.temperature = temp;
        record.humidity    = hum;
        record.pm1_0       = g_PM1_0_Value;
        record.pm2_5       = pm25_val;
        record.pm10_0      = g_PM10_0_Value;
        record.alcohol_mgL = alcohol_mgL;
        record.co2         = co2_val;
        record.alarm_type  = alarm_type;

        if (SD_Card_LogSensorData(&record) != SD_CARD_OK) {
            printf("SD Card Log Write Failed!\r\n");
        }
    }

    //蓝牙发送时间戳
    sprintf(ble_buf, "RunTime:%15s\r\n", time_stamp); 
    JDY23_SendString(ble_buf);
    memset(ble_buf, 0, sizeof(ble_buf));

    // 第二步：发送温湿度+PM2.5
    if(pm25_val == 0) {//Temp: 占10字符宽度右对齐 | Humidity: 占10字符宽度右对齐 | PM2.5: 固定提示
        sprintf(ble_buf, "Temp:%18.2f C\rHumidity:%12.2f%%\rPM2.5:\t\t  waiting...\r\n", temp, hum);
    } else {
        sprintf(ble_buf, "Temp:%18.2f C\rHumidity:%12.2f%%\rPM2.5:%14d ug/m3\r\n", temp, hum, pm25_val);
    }
    JDY23_SendString(ble_buf);
    memset(ble_buf, 0, sizeof(ble_buf));

    // 第三步：发送酒精+CO2
    if(co2_val == 0) {
        sprintf(ble_buf, "Alcohol:%14.2f mg/L\rCO2:\t\t  waiting...\r\n", alcohol_mgL);
    }else {
        sprintf(ble_buf, "Alcohol:%14.2f mg/L\rCO2:%19dppm \r\n", alcohol_mgL, co2_val);
    }
    JDY23_SendString(ble_buf);
    memset(ble_buf, 0, sizeof(ble_buf));

    // 第四步：发送分隔尾
    sprintf(ble_buf, "=================\r\n");
    JDY23_SendString(ble_buf);

    // OLED显示前加总线释放延时，避免和SCD41冲突
    HAL_Delay(5); 
    // 调用OLED显示传感器数据（传入报警类型）
    Sensor_Show_OLED(temp, hum, pm25_val, alcohol_mgL, co2_val, alarm_type);
    HAL_Delay(5); // 显示完成后再释放总线

    // 7. 蜂鸣器报警（根据报警类型触发）
    if (alarm_type != 0) {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET); // LED报警指示（PE1）
        Buzzer_Sensor_Alarm(1, 0, ALARM_DURATION); // 有报警时触发蜂鸣器
        alarm_type = 0; // 重置报警类型，等待下一次采集判断  
    }else {
        HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_RESET); // 无报警时关闭LED
    }
}

/* ====================== printf重定向（如需全局使用可放这里） ====================== */
#ifdef __GNUC__
#define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
#define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE {
    HAL_UART_Transmit(&DEBUG_UART_HANDLE, (uint8_t *)&ch, 1, 0xFFFF);
    return ch;
}

/* ====================== 串口中断回调（PMS5003） ====================== */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) { // PMS5003使用串口2
        PMS5003_ReceiveData(UART2_RxData); // 调用PMS5003接收处理函数
        HAL_UART_Receive_IT(&PMS5003_UART_HANDLE, &UART2_RxData, 1); // 重新开启中断
    }
}