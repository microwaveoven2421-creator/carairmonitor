# carairmonitor
My Graduation Project. Finished in 25/4/2026.

智能车载空气监测系统代码说明
作者：Microwave Oven

1. 项目介绍
本项目基于 STM32F407VET6，实现车载环境空气质量监测。系统通过多个传感器采集温湿度、PM2.5、CO2 和酒精浓度，并将结果显示在 OLED 上，同时通过蓝牙发送到外部设备，异常时触发 LED 和蜂鸣器报警，并将数据保存到 SD 卡。

2. 硬件与外设分配
模块	传感器/外设	通信方式	代码位置
温湿度	SHT31	I2C2	Core/Src/sht31.c, Core/Src/sht3x_i2c.c
颗粒物	PMS5003	USART2, 9600bps	Core/Src/PMS5003.c
二氧化碳	SCD41	I2C1	Core/Src/scd41.c, Core/Src/scd4x_i2c.c
酒精浓度	MQ-3	ADC1 CH0	Core/Src/mq3.c
显示	OLED	I2C1	Core/Src/oled.c
蓝牙	JDY-23	USART6	Core/Src/JDY23.c
存储	SD 卡	SPI3 + FatFs	Core/Src/sd_spi.c, Core/Src/sdcard_app.c
报警	蜂鸣器 + LED	TIM3 PWM + GPIO	Core/Src/buzzer.c, Core/Src/sensor_manager.c
3. 主程序流程
主入口在 Core/Src/main.c。

系统启动后，先完成 HAL 初始化、系统时钟配置和各外设初始化：

HAL_Init();
SystemClock_Config();
MX_GPIO_Init();
MX_ADC1_Init();
MX_I2C1_Init();
MX_I2C2_Init();
MX_SPI3_Init();
MX_TIM3_Init();
MX_USART1_UART_Init();
MX_USART2_UART_Init();
MX_USART6_UART_Init();
MX_FATFS_Init();
然后进入应用层初始化：

Sensor_Manager_Init();
主循环中每 5 秒采集一次数据：

while (1)
{
    Sensor_Manager_Collect_Alarm();
    HAL_Delay(COLLECT_INTERVAL_MS);
}
答辩时可以这样解释：
底层外设由 CubeMX 生成初始化函数，业务逻辑统一放在 sensor_manager.c，这样主函数保持简洁，系统结构清晰。

4. 系统核心调度模块
核心文件是 Core/Src/sensor_manager.c。

4.1 初始化函数
Sensor_Manager_Init() 完成全部业务模块初始化：

初始化 OLED，显示启动提示。
初始化 SHT31、PMS5003、SCD41。
开启 PMS5003 串口中断接收。
初始化蜂鸣器。
对 MQ-3 酒精传感器进行启动零点校准。
初始化 SD 卡。
初始化蓝牙模块。
其中 PMS5003 使用串口中断接收：

HAL_UART_Receive_IT(&PMS5003_UART_HANDLE, &UART2_RxData, 1);
这表示 USART2 每收到 1 个字节就进入中断回调，逐字节拼接 PMS5003 数据帧。

4.2 数据采集与报警函数
Sensor_Manager_Collect_Alarm() 是系统最重要的周期任务，主要做 6 件事：

读取 SCD41 的 CO2 数据。
读取 SHT31 的温湿度数据。
读取 PMS5003 的 PM2.5 数据。
读取 MQ-3 的酒精 ADC 值并转换为 mg/L。
判断是否报警。
输出到串口、蓝牙、OLED 和 SD 卡。
系统将所有报警类型放在同一个 alarm_type 变量中：

位值	含义
0x01	温度报警
0x02	湿度报警
0x04	酒精报警
0x08	PM2.5 报警
0x10	CO2 报警
如果多个异常同时发生，就通过按位或叠加。例如酒精和 PM2.5 同时报警时：

alarm_type = 0x04 | 0x08;
5. 连续两次异常才报警的设计
为了避免传感器瞬时波动导致误报警，代码中增加了连续异常确认机制：

static uint8_t Sensor_Alarm_Confirmed(uint8_t is_abnormal, uint8_t *counter)
逻辑是：

如果本次异常，计数器加 1。
如果本次正常，计数器清零。
当连续异常次数达到 ALARM_CONFIRM_COUNT，才真正报警。
当前配置为：

#define ALARM_CONFIRM_COUNT 2
答辩时可以说明：
传感器数据可能存在抖动，连续两次异常确认能过滤单次噪声，提高报警可靠性。

6. 各传感器代码解释
6.1 SHT31 温湿度传感器
相关文件：

Core/Src/sht31.c
Core/Src/sht3x_i2c.c
SHT31 通过 I2C2 通信。底层 sht3x_i2c.c 负责发送命令、读取原始数据和 CRC 校验。

温度换算公式在 sht3x_read_measurement() 中：

*temperature_m_deg_c = ((175000LL * raw_temperature) / 65535) - 45000;
这个公式来自 SHT3x 数据手册：

T = -45 + 175 * raw / 65535
为了减少传感器自热，测量频率从 10Hz 调整为 0.5Hz。因为主循环是 5 秒采集一次，0.5Hz 已经足够。

温度偏高的问题通过软件校准修正：

#define SHT31_TEMP_OFFSET_C -2.0f
如果实测仍偏高，可以继续调整这个偏移量。

6.2 PMS5003 PM2.5 传感器
相关文件：

Core/Src/PMS5003.c
Core/Inc/PMS5003.h
PMS5003 使用 USART2，波特率为 9600。传感器主动上报数据，数据帧以 0x42 0x4D 开头，总长度 32 字节。

接收过程使用状态机：

PMS5003_STATE_IDLE
PMS5003_STATE_MATCH_START1
PMS5003_STATE_RECEIVE_DATA
PMS5003_STATE_DATA_COMPLETE
串口中断回调在 sensor_manager.c：

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2) {
        PMS5003_ReceiveData(UART2_RxData);
        HAL_UART_Receive_IT(&PMS5003_UART_HANDLE, &UART2_RxData, 1);
    }
}
答辩时可以说明：
PMS5003 数据是连续串口流，所以不能一次简单读取固定值，而是用中断逐字节接收，再通过状态机识别完整帧。

6.3 SCD41 CO2 传感器
相关文件：

Core/Src/scd41.c
Core/Src/scd4x_i2c.c
SCD41 使用 I2C1 通信，代码支持周期性测量和单次测量。当前通过宏选择测量方式：

#define SCD41_MEASURE_MODE 0
SCD41_Read_CO2() 根据模式调用：

return SCD41_Read_CO2_Periodic();
周期模式下，如果数据暂时没准备好，就返回 0，系统显示 waiting...，下个采集周期再读。

6.4 MQ-3 酒精传感器
相关文件：

Core/Src/mq3.c
Core/Src/sensor_manager.c
MQ-3 输出模拟电压，接到 ADC1 通道 0。代码先读取 ADC 原始值，再转换为酒精浓度：

mq3_val = MQ3_Get_AlcoholConcentration_Average(ADC_CHANNEL_0, 10);
float alcohol_mgL = MQ3_Convert_To_MgL(mq3_val);
为了降低无酒精环境下显示偏高，系统增加了启动零点校准：

mq3_baseline_adc = MQ3_Get_AlcoholConcentration_Average(ADC_CHANNEL_0, 20);
转换时使用基线和死区：

if (adc_val <= mq3_baseline_adc + MQ3_NOISE_DEADBAND_ADC) {
    return 0.0f;
}
答辩时可以说明：
MQ-3 是模拟气体传感器，受预热时间、环境和模块个体差异影响较大，所以代码采用启动基线校准和死区处理，减少误判。

7. 显示、蓝牙、SD 卡与报警输出
7.1 OLED 显示
文件：Core/Src/oled.c

OLED 显示四行数据：

温度 + 湿度
PM2.5
酒精浓度
CO2
如果对应传感器报警，会在行尾显示 !。

7.2 蓝牙发送
文件：Core/Src/JDY23.c

蓝牙模块通过 USART6 发送字符串。系统每次采集后将运行时间、温湿度、PM2.5、酒精和 CO2 发送出去：

JDY23_SendString(ble_buf);
7.3 SD 卡数据记录
文件：

Core/Src/sd_spi.c
Core/Src/sdcard_app.c
SD 卡日志文件为：

sensor_log.csv
字段格式：

time_stamp,temperature_c,humidity_rh,pm1_0,pm2_5,pm10_0,alcohol_mgL,co2_ppm,alarm_type
代码通过结构体组织一条记录：

SD_SensorRecord_t record;
record.temperature = temp;
record.humidity = hum;
record.pm2_5 = pm25_val;
record.alcohol_mgL = alcohol_mgL;
record.co2 = co2_val;
record.alarm_type = alarm_type;
然后写入 SD 卡：

SD_Card_LogSensorData(&record);
7.4 蜂鸣器与 LED 报警
文件：

Core/Src/buzzer.c
Core/Src/sensor_manager.c
如果 alarm_type != 0，说明有传感器连续异常：

HAL_GPIO_WritePin(GPIOE, GPIO_PIN_3, GPIO_PIN_SET);
Buzzer_Sensor_Alarm(1, 0, ALARM_DURATION);
蜂鸣器通过 TIM3 PWM 输出声音，LED 用 GPIO 指示报警状态。

8. 项目代码结构优点
8.1 分层清晰
代码大致分为三层：

外设初始化层：main.c, usart.c, i2c.c, spi.c, adc.c
传感器驱动层：sht31.c, PMS5003.c, scd41.c, mq3.c
应用管理层：sensor_manager.c
这样的结构让主函数很简洁，传感器细节也不会混在一起。

8.2 报警逻辑集中
所有报警阈值都集中在 sensor_manager.h 和 sensor_manager.c 中，便于统一调整：

#define CO2_ALARM_THRESHOLD 1500
#define PM25_ALARM_THRESHOLD 75
#define TEMP_ALARM_LOW 0.0f
#define TEMP_ALARM_HIGH 35.0f
#define HUM_ALARM_LOW 15.0f
#define HUM_ALARM_HIGH 70.0f
8.3 数据输出完整
同一组采集数据会同时输出到：

串口调试
OLED 显示
蓝牙发送
SD 卡保存
蜂鸣器/LED 报警
这使系统既能本地查看，也能外部接收和离线追溯。
