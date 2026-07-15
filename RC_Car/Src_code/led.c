/**
  ******************************************************************************
  * @file    led.c
  * @brief   RGB 상태 LED 제어
  * @details 공통 음극 RGB LED를 사용해 주행 모드를 색상으로 표시한다.
  ******************************************************************************
  */

#include "led.h"

/**
  * @brief RGB LED를 모두 끈 상태로 초기화한다.
  */
void LED_Init(void) {
    LED_SetColor(LED_COLOR_OFF);
}

/**
  * @brief 지정한 RGB 색상을 출력한다.\n  * @param color 출력할 LED 색상
  */
void LED_SetColor(LED_Color_t color) {
    /* CubeMX User Label로 생성된 Port/Pin 매크로를 사용한다. */
    switch(color) {
        case LED_COLOR_BLUE: // 파란색
            HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);
            break;

        case LED_COLOR_GREEN: // 초록색
            HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);
            break;

        case LED_COLOR_RED: // 빨간색
            HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);
            break;

        case LED_COLOR_WHITE: // 흰색 (R,G,B 동시 점등)
            HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_SET);
            HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_SET);
            break;

        case LED_COLOR_OFF: // 모두 끄기
        default:
            HAL_GPIO_WritePin(LED_R_GPIO_Port, LED_R_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_G_GPIO_Port, LED_G_Pin, GPIO_PIN_RESET);
            HAL_GPIO_WritePin(LED_B_GPIO_Port, LED_B_Pin, GPIO_PIN_RESET);
            break;
    }
}

/**
  * @brief 주행 모드를 상태 LED 색상으로 변환한다.\n  * @param mode 0: Manual, 1: Auto, 2: Controller
  */
void LED_UpdateByMode(uint8_t mode) {
    switch(mode) {
        case 0:  LED_SetColor(LED_COLOR_BLUE);  break; // Manual Mode
        case 1:  LED_SetColor(LED_COLOR_GREEN); break; // Auto Mode
        case 2:  LED_SetColor(LED_COLOR_RED);   break; // Controller Mode
        default: LED_SetColor(LED_COLOR_OFF);  break;
    }
}
