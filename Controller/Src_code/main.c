/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
uint16_t adc_buf[4];   // [0]:Joy1_X, [1]:Joy1_Y, [2]:Joy2_X, [3]:Joy2_Y
char current_cmd = 's'; // 현재 입력된 명령어 (초기값: Stop)
char last_cmd = 's';    // 직전에 전송한 명령어
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
uint8_t prev_btn1 = GPIO_PIN_SET; // 경적 버튼 직전 상태 (풀업이므로 기본 SET)
uint8_t prev_btn2 = GPIO_PIN_SET; // 라이트 버튼 직전 상태 (풀업이므로 기본 SET)
uint8_t curr_btn1 = GPIO_PIN_SET; // 경적 버튼 현재 상태 (전역 - 디버그 Live Expressions 확인용)
uint8_t curr_btn2 = GPIO_PIN_SET; // 라이트 버튼 현재 상태 (전역 - 디버그 Live Expressions 확인용)
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
	HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buf, 4);
	/* USER CODE END 2 */

	/* Infinite loop */
	/* USER CODE BEGIN WHILE */
	while (1)
	{
		// 1. 필요한 채널의 최신 아날로그 값 가져오기
		uint16_t joy1_y = adc_buf[1]; // 좌측 조이스틱 상하 (전진/후진)
		uint16_t joy2_x = adc_buf[2]; // 우측 조이스틱 좌우 (좌회전/우회전)

		// 2. 임계값(Threshold) 기준 방향 판별 로직
		if (joy1_y > 3000) {
			current_cmd = 'f'; // 전진 (Forward)
		}
		else if (joy1_y < 1000) {
			current_cmd = 'b'; // 후진 (Backward)
		}
		else if (joy2_x > 3000) {
			current_cmd = 'l'; // 우회전 (Right)
		}
		else if (joy2_x < 1000) {
			current_cmd = 'r'; // 좌회전 (Left)
		}
		else {
			current_cmd = 's'; // 입력 없음 -> 정지 (Stop)
		}

		// 조이스틱 1 버튼 (경적 - 'H')
		// PB12: GPIO_Input, Pull-up 으로 설정되어 있어야 함
		curr_btn1 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_12);
		if (curr_btn1 == GPIO_PIN_RESET && prev_btn1 == GPIO_PIN_SET) { // 안 눌려있다가 눌린 순간!
			uint8_t tx_data = 'H';
			HAL_UART_Transmit(&huart1, &tx_data, 1, 10);
			HAL_Delay(50); // 버튼 채터링(노이즈) 방지용 짧은 대기
		}
		prev_btn1 = curr_btn1; // 상태 업데이트

		// 조이스틱 2 버튼 (라이트 - 'L')
		// PB13: GPIO_Input, Pull-up 으로 설정되어 있어야 함
		curr_btn2 = HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_13);
		if (curr_btn2 == GPIO_PIN_RESET && prev_btn2 == GPIO_PIN_SET) {
			uint8_t tx_data = 'T';
			HAL_UART_Transmit(&huart1, &tx_data, 1, 10);
			HAL_Delay(50);
		}
		prev_btn2 = curr_btn2;

		// 3. 데이터 폭주 방지 로직 (명령어가 바뀔 때만 딱 한 번 UART 송신)
		if (current_cmd != last_cmd) {
			HAL_UART_Transmit(&huart1, (uint8_t*)&current_cmd, 1, 10);
			last_cmd = current_cmd; // 최근 전송한 상태 업데이트
		}

		// 4. 무한 루프 과부하 방지 및 입력 감도 최적화 대기 (50ms)
		HAL_Delay(50);
	}
	/* USER CODE END WHILE */

	/* USER CODE BEGIN 3 */
}
/* USER CODE END 3 */


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
