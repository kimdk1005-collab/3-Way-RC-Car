/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Dual Joystick Controller 입력 처리 및 UART 명령 송신
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */

/* ADC1 Scan + Circular DMA 결과: Joy1_X, Joy1_Y, Joy2_X, Joy2_Y */
uint16_t adc_buf[4];

/* 현재 판정된 방향 명령과 마지막으로 전송한 방향 명령 */
char current_cmd = 's';
char last_cmd = 's';

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/*
 * PB12/PB13은 Pull-up 입력이므로 미입력은 SET, 버튼 입력은 RESET이다.
 * 직전 상태와 현재 상태를 비교해 Falling Edge에서 한 번만 명령을 전송한다.
 */
uint8_t prev_btn1 = GPIO_PIN_SET;
uint8_t prev_btn2 = GPIO_PIN_SET;
uint8_t curr_btn1 = GPIO_PIN_SET;
uint8_t curr_btn2 = GPIO_PIN_SET;

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  /*
   * ADC1 4개 채널을 Circular DMA로 연속 수집한다.
   * CPU가 직접 채널을 순회하지 않아도 adc_buf에 최신값이 유지된다.
   */
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buf, 4);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
      /* 실제 주행 판정에 사용하는 두 축의 최신 DMA 값을 읽는다. */
      uint16_t joy1_y = adc_buf[1];
      uint16_t joy2_x = adc_buf[2];

      /*
       * Joystick 1 Y축을 전진/후진에 우선 배정한다.
       * Y축이 중립일 때만 Joystick 2 X축으로 좌/우 명령을 판정한다.
       */
      if (joy1_y > 3000U)
      {
          current_cmd = 'f';
      }
      else if (joy1_y < 1000U)
      {
          current_cmd = 'b';
      }
      else if (joy2_x > 3000U)
      {
          /*
           * 현재 PCB 장착/배선 방향에서 이 구간은 'l'을 전송한다.
           * 물리적 체감 방향이 반대라면 두 조건의 명령 문자만 교환한다.
           */
          current_cmd = 'l';
      }
      else if (joy2_x < 1000U)
      {
          current_cmd = 'r';
      }
      else
      {
          current_cmd = 's';
      }

      /* Joystick 1 버튼(PB12): 눌리는 순간 경적 명령 'H' 전송 */
      curr_btn1 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);

      if ((curr_btn1 == GPIO_PIN_RESET) && (prev_btn1 == GPIO_PIN_SET))
      {
          uint8_t tx_data = 'H';
          HAL_UART_Transmit(&huart1, &tx_data, 1, 10);
          HAL_Delay(50); /* 간단한 버튼 채터링 억제 */
      }

      prev_btn1 = curr_btn1;

      /*
       * Joystick 2 버튼(PB13): 헤드라이트 토글 명령 'T' 전송
       * 'L'은 좌회전 명령으로 사용되므로 조명 명령과 분리한다.
       */
      curr_btn2 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13);

      if ((curr_btn2 == GPIO_PIN_RESET) && (prev_btn2 == GPIO_PIN_SET))
      {
          uint8_t tx_data = 'T';
          HAL_UART_Transmit(&huart1, &tx_data, 1, 10);
          HAL_Delay(50); /* 간단한 버튼 채터링 억제 */
      }

      prev_btn2 = curr_btn2;

      /* 방향 상태가 바뀔 때만 전송해 불필요한 UART 트래픽을 줄인다. */
      if (current_cmd != last_cmd)
      {
          HAL_UART_Transmit(&huart1, (uint8_t*)&current_cmd, 1, 10);
          last_cmd = current_cmd;
      }

      /* 입력 판정 주기: 50 ms */
      HAL_Delay(50);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 100;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
