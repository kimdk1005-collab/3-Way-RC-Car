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
#define IN4_Pin GPIO_PIN_0
#define IN4_GPIO_Port GPIOC
#define IN3_Pin GPIO_PIN_1
#define IN3_GPIO_Port GPIOC
#define LED_REAR_Pin GPIO_PIN_3
#define LED_REAR_GPIO_Port GPIOC
#define ENA_Pin GPIO_PIN_0
#define ENA_GPIO_Port GPIOA
#define ENB_Pin GPIO_PIN_1
#define ENB_GPIO_Port GPIOA
#define IN1_Pin GPIO_PIN_4
#define IN1_GPIO_Port GPIOA
#define TRIG_C_Pin GPIO_PIN_6
#define TRIG_C_GPIO_Port GPIOA
#define ECHO_C_Pin GPIO_PIN_7
#define ECHO_C_GPIO_Port GPIOA
#define IN2_Pin GPIO_PIN_0
#define IN2_GPIO_Port GPIOB
#define LED_FRONT_R_Pin GPIO_PIN_7
#define LED_FRONT_R_GPIO_Port GPIOC
#define BUZZER_Pin GPIO_PIN_8
#define BUZZER_GPIO_Port GPIOC
#define LED_FRONT_L_Pin GPIO_PIN_9
#define LED_FRONT_L_GPIO_Port GPIOC
#define BT_TX_Pin GPIO_PIN_9
#define BT_TX_GPIO_Port GPIOA
#define BT_RX_Pin GPIO_PIN_10
#define BT_RX_GPIO_Port GPIOA
#define LED_R_Pin GPIO_PIN_10
#define LED_R_GPIO_Port GPIOC
#define LED_G_Pin GPIO_PIN_11
#define LED_G_GPIO_Port GPIOC
#define LED_B_Pin GPIO_PIN_12
#define LED_B_GPIO_Port GPIOC
#define TRIG_R_Pin GPIO_PIN_4
#define TRIG_R_GPIO_Port GPIOB
#define ECHO_R_Pin GPIO_PIN_5
#define ECHO_R_GPIO_Port GPIOB
#define TRIG_L_Pin GPIO_PIN_8
#define TRIG_L_GPIO_Port GPIOB
#define ECHO_L_Pin GPIO_PIN_9
#define ECHO_L_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
