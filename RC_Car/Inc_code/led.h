#ifndef INC_LED_H_
#define INC_LED_H_

#include "main.h"

// LED 색상(모드) 상태 정의
typedef enum {
    LED_COLOR_OFF = 0,
    LED_COLOR_BLUE,   // Manual Mode (수동 스마트폰 조종)
    LED_COLOR_GREEN,  // Auto Mode (초음파 자율주행)
    LED_COLOR_RED,    // Controller Mode (물리 조종기 대기/연결)
    LED_COLOR_WHITE   // 시스템 초기화/테스트용
} LED_Color_t;

// 함수 선언
void LED_Init(void);
void LED_SetColor(LED_Color_t color);
void LED_UpdateByMode(uint8_t mode);

#endif /* INC_LED_H_ */
