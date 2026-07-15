#include "led.h"

/**
  * @brief LED 초기화 (부팅 시 전원 OFF 상태로 시작)
  */
void LED_Init(void) {
    LED_SetColor(LED_COLOR_OFF);
}

/**
  * @brief RGB LED 색상 강제 지정 함수 (공통 음극 - Common Cathode 기준)
  */
void LED_SetColor(LED_Color_t color) {
    // CubeMX에서 지정한 User Label 매크로(LED_R_Pin 등)를 사용합니다.
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
  * @brief car_mode 변수 값을 읽어서 자동으로 LED 색상을 매핑해주는 함수
  */
void LED_UpdateByMode(uint8_t mode) {
    switch(mode) {
        case 0:  LED_SetColor(LED_COLOR_BLUE);  break; // Manual Mode
        case 1:  LED_SetColor(LED_COLOR_GREEN); break; // Auto Mode
        case 2:  LED_SetColor(LED_COLOR_RED);   break; // Controller Mode
        default: LED_SetColor(LED_COLOR_OFF);  break;
    }
}
