/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Controller_V1 - 조이스틱 → HC-05/HM-10 BLE → RC카 전송
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "adc.h"
#include "dma.h"
#include "usart.h"
#include "gpio.h"

/* USER CODE BEGIN PV */
// DMA Circular 모드로 자동 갱신됨
// Rank 순서: [0]=PA0(CH0), [1]=PA1(CH1), [2]=PA2(CH2), [3]=PA3(CH3)
uint16_t adc_buf[4];   // [0]:Joy1_X, [1]:Joy1_Y, [2]:Joy2_X, [3]:Joy2_Y

char current_cmd = 's'; // 현재 판별된 명령어 (초기값: 정지)
// last_cmd 변수는 무선 통신 신뢰성을 위해 제거되었습니다.
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();       // ★ ADC보다 반드시 먼저 초기화
  MX_ADC1_Init();
  MX_USART1_UART_Init();

  /* USER CODE BEGIN 2 */
  // DMA Circular 모드로 4채널 ADC 연속 변환 시작
  // 이후 adc_buf[]는 자동으로 최신값 유지 (폴링 불필요)
  HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_buf, 4);
  /* USER CODE END 2 */

  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // ── 1. ADC 최신값 읽기 ──────────────────────────────────
    // DMA가 백그라운드에서 자동 갱신하므로 그냥 읽기만 하면 됨
    // ★ 배선 확인 후 인덱스 조정 필요 (Live Expression으로 확인)
    uint16_t joy1_y = adc_buf[1]; // 좌측 조이스틱 상하 → 전진/후진
    uint16_t joy2_x = adc_buf[2]; // 우측 조이스틱 좌우 → 좌/우회전

    // ── 2. 방향 판별 (Y축 우선, X축 후순위) ──────────────────
    // 조이스틱 중립값 약 2048 기준 (12bit ADC: 0~4095)
    if      (joy1_y > 3000) current_cmd = 'f'; // 전진
    else if (joy1_y < 1000) current_cmd = 'b'; // 후진
    else if (joy2_x > 3000) current_cmd = 'r'; // 우회전
    else if (joy2_x < 1000) current_cmd = 'l'; // 좌회전
    else                    current_cmd = 's'; // 정지

    // ── 3. 무조건 UART 전송 (무선 신뢰성 확보) ───────────────
    // 상태가 변할 때만이 아니라, 현재 상태를 50ms마다 계속 전송합니다.
    // 이렇게 해야 RC카가 무선 노이즈로 문자를 한 번 놓치더라도
    // 바로 다음 번에 문자를 받아 정상 작동(특히 정지)할 수 있습니다.
    HAL_UART_Transmit(&huart1, (uint8_t*)&current_cmd, 1, 10);

    // ── 4. 50ms 대기 (20Hz 폴링 및 전송 주기) ────────────────
    HAL_Delay(50);
  }
  /* USER CODE END WHILE */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState       = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState   = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource  = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 25;
  RCC_OscInitStruct.PLL.PLLN = 200;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType      = RCC_CLOCKTYPE_HCLK  | RCC_CLOCKTYPE_SYSCLK
                                   | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider  = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;
  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_3) != HAL_OK)
  {
    Error_Handler();
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {}
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
}
#endif /* USE_FULL_ASSERT */
