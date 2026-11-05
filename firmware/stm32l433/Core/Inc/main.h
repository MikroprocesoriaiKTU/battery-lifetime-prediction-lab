/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
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
#include "stm32l4xx_hal.h"

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
#define B1_Pin GPIO_PIN_13
#define B1_GPIO_Port GPIOC
#define MCO_Pin GPIO_PIN_0
#define MCO_GPIO_Port GPIOH
#define I2C3_SCL_Pin GPIO_PIN_0
#define I2C3_SCL_GPIO_Port GPIOC
#define I2C3_SDA_Pin GPIO_PIN_1
#define I2C3_SDA_GPIO_Port GPIOC
#define ADC_R5_Pin GPIO_PIN_2
#define ADC_R5_GPIO_Port GPIOC
#define ADC_R6_Pin GPIO_PIN_3
#define ADC_R6_GPIO_Port GPIOC
#define ADC_DRIVE_Pin GPIO_PIN_0
#define ADC_DRIVE_GPIO_Port GPIOA
#define ADC_DIVIDER_Pin GPIO_PIN_1
#define ADC_DIVIDER_GPIO_Port GPIOA
#define LD4_Pin GPIO_PIN_13
#define LD4_GPIO_Port GPIOB
#define LED1_Pin GPIO_PIN_9
#define LED1_GPIO_Port GPIOA
#define LED2_Pin GPIO_PIN_10
#define LED2_GPIO_Port GPIOA
#define SWITCH_S1_Pin GPIO_PIN_11
#define SWITCH_S1_GPIO_Port GPIOA
#define OLED_VCC_Pin GPIO_PIN_12
#define OLED_VCC_GPIO_Port GPIOA
#define TMS_Pin GPIO_PIN_13
#define TMS_GPIO_Port GPIOA
#define TCK_Pin GPIO_PIN_14
#define TCK_GPIO_Port GPIOA
#define OscTestPoint_Pin GPIO_PIN_10
#define OscTestPoint_GPIO_Port GPIOC
#define SWO_Pin GPIO_PIN_3
#define SWO_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
