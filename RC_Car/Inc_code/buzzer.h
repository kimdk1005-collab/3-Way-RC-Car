#ifndef BUZZER_H_
#define BUZZER_H_

#include "main.h"

// 부저 상태 열거형
typedef enum {
    BUZZER_IDLE,         // 대기 (소리 없음)
    BUZZER_HORN,         // 클락션 (계속 소리 남)
    BUZZER_REVERSE_BEEP  // 후진 경고음 (깜빡임)
} BuzzerState;

// 외부에서 사용할 수 있는 함수들 (API)
void Buzzer_SetState(BuzzerState state); // 부저의 상태를 변경하는 함수
void Buzzer_Update(void);                // 부저 상태를 실시간으로 체크하는 함수

#endif /* BUZZER_H_ */
