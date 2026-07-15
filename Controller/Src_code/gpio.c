/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    gpio.c
  * @brief   전용 컨트롤러 버튼 GPIO 초기화
  * @details PB12는 경적, PB13은 헤드라이트 토글 버튼이며 두 입력 모두
  *          내부 Pull-up을 사용한다.
  ******************************************************************************
  */
/* USER CODE END Header */

#include "gpio.h"

/**
  * @brief 사용 GPIO Port Clock과 버튼 입력을 초기화한다.
  * @note  Pull-up 입력이므로 버튼 미입력은 SET, 입력은 RESET이다.
  */
void MX_GPIO_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PB12: Horn button, PB13: Headlight toggle button */
    GPIO_InitStruct.Pin = GPIO_PIN_12 | GPIO_PIN_13;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}
