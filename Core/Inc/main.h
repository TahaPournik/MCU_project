/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
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
#include "stm32f1xx_hal.h"

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
#define LED_POWER_Pin GPIO_PIN_13
#define LED_POWER_GPIO_Port GPIOC
#define LED_FAULT_Pin GPIO_PIN_0
#define LED_FAULT_GPIO_Port GPIOA
#define RS485_DE_Pin GPIO_PIN_1
#define RS485_DE_GPIO_Port GPIOA
#define RS485_TX_Pin GPIO_PIN_2
#define RS485_TX_GPIO_Port GPIOA
#define RS485_RX_Pin GPIO_PIN_3
#define RS485_RX_GPIO_Port GPIOA
#define MAX31865_CS_Pin GPIO_PIN_4
#define MAX31865_CS_GPIO_Port GPIOA
#define MAX31865_SCK_Pin GPIO_PIN_5
#define MAX31865_SCK_GPIO_Port GPIOA
#define MAX31865_MISO_Pin GPIO_PIN_6
#define MAX31865_MISO_GPIO_Port GPIOA
#define MAX31865_MOSI_Pin GPIO_PIN_7
#define MAX31865_MOSI_GPIO_Port GPIOA
#define VC_MONITOR_Pin GPIO_PIN_0
#define VC_MONITOR_GPIO_Port GPIOB
#define VC_MONITOR_EXTI_IRQn EXTI0_IRQn
#define FURNACE_RELAY_Pin GPIO_PIN_1
#define FURNACE_RELAY_GPIO_Port GPIOB
#define FAN_RELAY_Pin GPIO_PIN_2
#define FAN_RELAY_GPIO_Port GPIOB
#define UP_BTN_Pin GPIO_PIN_10
#define UP_BTN_GPIO_Port GPIOB
#define DOWN_BTN_Pin GPIO_PIN_11
#define DOWN_BTN_GPIO_Port GPIOB
#define MAX31855_CS_Pin GPIO_PIN_12
#define MAX31855_CS_GPIO_Port GPIOB
#define MAX31855_SCK_Pin GPIO_PIN_13
#define MAX31855_SCK_GPIO_Port GPIOB
#define MAX31855_MISO_Pin GPIO_PIN_14
#define MAX31855_MISO_GPIO_Port GPIOB
#define LCD_D7_Pin GPIO_PIN_15
#define LCD_D7_GPIO_Port GPIOB
#define LED_COM_Pin GPIO_PIN_8
#define LED_COM_GPIO_Port GPIOA
#define MENU_BTN_Pin GPIO_PIN_9
#define MENU_BTN_GPIO_Port GPIOA
#define MENU_BTN_EXTI_IRQn EXTI9_5_IRQn
#define ENTER_BTN_Pin GPIO_PIN_10
#define ENTER_BTN_GPIO_Port GPIOA
#define ENTER_BTN_EXTI_IRQn EXTI15_10_IRQn
#define LCD_RS_Pin GPIO_PIN_5
#define LCD_RS_GPIO_Port GPIOB
#define LCD_E_Pin GPIO_PIN_6
#define LCD_E_GPIO_Port GPIOB
#define LCD_D4_Pin GPIO_PIN_7
#define LCD_D4_GPIO_Port GPIOB
#define LCD_D5_Pin GPIO_PIN_8
#define LCD_D5_GPIO_Port GPIOB
#define LCD_D6_Pin GPIO_PIN_9
#define LCD_D6_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
