#include "buzzer.h"
#include "tim.h" // ⭐️ 타이머(htim3) 변수를 가져오기 위해 추가

// 타이머 변수 외부 참조
extern TIM_HandleTypeDef htim3;

static BuzzerState currentBuzzerState = BUZZER_IDLE;
static uint32_t buzzerLastTick = 0;
static uint8_t beepToggle = 0;
static uint32_t hornStartTime = 0;

static void Buzzer_SetTone(uint32_t freq_hz)
{
    uint32_t timer_clk_hz = 1000000U;
    uint32_t period;

    if (freq_hz == 0U)
    {
        return;
    }

    period = (timer_clk_hz / freq_hz) - 1U;

    __HAL_TIM_SET_AUTORELOAD(&htim3, period);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_3, (period + 1U) / 2U);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_3);
}

// ⭐️ 부저 켜기: 타이머 PWM 시작
static void Buzzer_On(void) {
    Buzzer_SetTone(400U);
}

// ⭐️ 부저 끄기: 타이머 PWM 정지
static void Buzzer_Off(void) {
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
}

// 상태 세팅 함수 (기존과 동일)
void Buzzer_SetState(BuzzerState state) {
    if (currentBuzzerState != state) {
        currentBuzzerState = state;

        if (state == BUZZER_IDLE) {
            Buzzer_Off();
        }
        else if (state == BUZZER_HORN) {
            hornStartTime = HAL_GetTick();
            Buzzer_On();
        }
    }
}

// 논블로킹 업데이트 함수 (기존과 동일)
void Buzzer_Update(void) {
    uint32_t currTick = HAL_GetTick();

    switch(currentBuzzerState) {
        case BUZZER_IDLE:
            break;

        case BUZZER_HORN:
            if (currTick - hornStartTime > 400) {
                Buzzer_SetState(BUZZER_IDLE);
            } else if (((currTick - hornStartTime) / 35U) % 2U == 0U) {
                Buzzer_SetTone(430U);
            } else {
                Buzzer_SetTone(480U);
            }
            break;

        case BUZZER_REVERSE_BEEP:
            if ((currTick - buzzerLastTick) >= 300) {
                buzzerLastTick = currTick;
                if (beepToggle == 0) {
                    Buzzer_On();
                    beepToggle = 1;
                } else {
                    Buzzer_Off();
                    beepToggle = 0;
                }
            }
            break;
    }
}
