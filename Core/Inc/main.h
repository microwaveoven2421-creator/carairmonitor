/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define MQ3_AO_Pin GPIO_PIN_0
#define MQ3_AO_GPIO_Port GPIOA
#define pms5003_TX_Pin GPIO_PIN_2
#define pms5003_TX_GPIO_Port GPIOA
#define pms5003_RX_Pin GPIO_PIN_3
#define pms5003_RX_GPIO_Port GPIOA
#define Buzzer_Pin GPIO_PIN_1
#define Buzzer_GPIO_Port GPIOB
#define sht31_scl_Pin GPIO_PIN_10
#define sht31_scl_GPIO_Port GPIOB
#define sht31_sda_Pin GPIO_PIN_11
#define sht31_sda_GPIO_Port GPIOB
#define ble_tx_Pin GPIO_PIN_6
#define ble_tx_GPIO_Port GPIOC
#define ble_rx_Pin GPIO_PIN_7
#define ble_rx_GPIO_Port GPIOC
#define SD_CS_Pin GPIO_PIN_11
#define SD_CS_GPIO_Port GPIOA
#define scd41_oled_scl_Pin GPIO_PIN_6
#define scd41_oled_scl_GPIO_Port GPIOB
#define scd41_oled_sda_Pin GPIO_PIN_7
#define scd41_oled_sda_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
