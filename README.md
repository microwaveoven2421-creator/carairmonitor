# 基于 STM32 的智能车载空气监控与联动系统

作者：Microwave Oven
项目完成时间：2026-04-25

## 项目简介

本项目基于 STM32F407VET6 开发，实现车载环境空气质量监测。系统通过多个传感器采集温湿度、PM2.5、CO2 和酒精浓度，并将结果显示在 OLED 上，同时通过蓝牙发送到外部设备，异常时触发 LED 和蜂鸣器报警，并将数据保存到 SD 卡。

项目主要用于嵌入式系统开发训练，覆盖传感器驱动、串口通信、I2C/SPI/ADC 外设使用、数据存储、显示输出和报警逻辑设计。

## 功能特点

- 多传感器数据采集：温湿度、PM1.0、PM2.5、PM10、CO2、酒精浓度
- OLED 实时显示监测数据
- JDY-23 蓝牙串口发送数据
- SD 卡本地 CSV 日志记录
- LED 与蜂鸣器报警提示
- 连续两次异常确认后报警，降低误报警概率
- MQ-3 启动零点校准，降低酒精浓度静态偏高
- SHT31 温度偏移校准，补偿安装位置导致的温度偏差

## 硬件与外设分配

| 模块 | 传感器/外设 | 通信方式 | 代码位置 |
| --- | --- | --- | --- |
| 温湿度 | SHT31 | I2C2 | `Core/Src/sht31.c`, `Core/Src/sht3x_i2c.c` |
| 颗粒物 | PMS5003 | USART2, 9600bps | `Core/Src/PMS5003.c` |
| 二氧化碳 | SCD41 | I2C1 | `Core/Src/scd41.c`, `Core/Src/scd4x_i2c.c` |
| 酒精浓度 | MQ-3 | ADC1 CH0 | `Core/Src/mq3.c` |
| 显示 | OLED | I2C1 | `Core/Src/oled.c` |
| 蓝牙 | JDY-23 | USART6 | `Core/Src/JDY23.c` |
| 存储 | SD 卡 | SPI3 + FatFs | `Core/Src/sd_spi.c`, `Core/Src/sdcard_app.c` |
| 报警 | 蜂鸣器 + LED | TIM3 PWM + GPIO | `Core/Src/buzzer.c`, `Core/Src/sensor_manager.c` |

## 系统架构图

![系统架构图](%E7%B3%BB%E7%BB%9F%E6%9E%B6%E6%9E%84%E5%9B%BE-carairmonitor.png)

## 系统流程图

![系统流程图](https://foruda.gitee.com/images/1774417259617906178/2967e83c_16552579.png "系统流程图-carairmonitor.png")

## 软件设计

### 1. 主程序

主入口位于 `Core/Src/main.c`。系统完成 HAL、时钟和外设初始化后，调用：

```c
Sensor_Manager_Init();
```

主循环中每 5 秒执行一次：

```c
Sensor_Manager_Collect_Alarm();
HAL_Delay(COLLECT_INTERVAL_MS);
```

### 2. 传感器管理

核心业务逻辑位于 `Core/Src/sensor_manager.c`，负责：

- 调度所有传感器采集
- 应用温度和酒精传感器校准
- 判断报警阈值
- 执行连续两次异常确认
- 更新 OLED 显示
- 发送蓝牙数据
- 写入 SD 卡日志
- 控制 LED 和蜂鸣器

### 3. SD 卡日志

SD 卡日志文件名为：

```text
sensor_log.csv
```

字段格式：

```text
time_stamp,temperature_c,humidity_rh,pm1_0,pm2_5,pm10_0,alcohol_mgL,co2_ppm,alarm_type
```

`alarm_type` 使用位标记：

| 值 | 含义 |
| --- | --- |
| `0` | 无报警 |
| `1` | 温度报警 |
| `2` | 湿度报警 |
| `4` | 酒精报警 |
| `8` | PM2.5 报警 |
| `16` | CO2 报警 |

多个报警同时发生时数值相加，例如 `12 = 酒精报警 + PM2.5 报警`。

## 项目难点

- 多传感器数据同步采集与异常处理
- PMS5003 串口字节流解析和帧校验
- SD 卡 SPI 驱动与 FatFs 文件系统适配
- 模拟传感器 MQ-3 的零点漂移和偏高问题处理
- 报警逻辑的稳定性设计

## 后续优化

- 引入 FreeRTOS 多任务调度
- 优化蓝牙数据协议
- 增加更多异常检测和故障恢复机制
- 增加风扇或净化模块联动控制
