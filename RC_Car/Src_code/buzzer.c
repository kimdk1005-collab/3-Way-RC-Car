/**
  ******************************************************************************
  * @file    buzzer.c
  * @brief   TIM3 PWM 기반 경적 및 후진 경고음 상태 제어
  * @details HAL_Delay()를 사용하지 않고 HAL_GetTick()으로 음향 패턴을
  *          갱신하여 MotorTask의 주행 제어를 차단하지 않는다.
  ******************************************************************************
  */

#include "buzzer.h"
#include "tim.h"

extern TIM_HandleTypeDef htim3;

/* 현재 부저 동작 상태 */
static BuzzerState currentBuzzerState = BUZZER_IDLE;

/* 후진 경고음 토글 시각과 출력 상태 */
static uint32_t buzzerLastTick = 0;
static uint8_t beepToggle = 0;

/* 경적 시작 시각 */
static uint32_t hornStartTime = 0;

/**
  * @brief TIM3 CH3의 PWM 주파수와 Duty 50%를 설정한다.
  * @param freq_hz 출력할 주파수 [Hz]
  */
static void Buzzer_SetTone(uint32_t freq_hz)
{
    const uint32_t timer_clk_hz = 1000000U;
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

/** @brief 기본 후진 경고음 주파수로 PWM 출력을 시작한다. */
static void Buzzer_On(void)
{
    Buzzer_SetTone(400U);
}

/** @brief TIM3 CH3 PWM 출력을 정지한다. */
static void Buzzer_Off(void)
{
    HAL_TIM_PWM_Stop(&htim3, TIM_CHANNEL_3);
}

/**
  * @brief 부저 상태를 변경한다.
  * @param state BUZZER_IDLE, BUZZER_HORN, BUZZER_REVERSE_BEEP
  */
void Buzzer_SetState(BuzzerState state)
{
    if (currentBuzzerState == state)
    {
        return;
    }

    currentBuzzerState = state;

    if (state == BUZZER_IDLE)
    {
        Buzzer_Off();
    }
    else if (state == BUZZER_HORN)
    {
        hornStartTime = HAL_GetTick();
        Buzzer_On();
    }
}

/**
  * @brief 현재 상태에 맞춰 부저 출력을 논블로킹 방식으로 갱신한다.
  * @note  MotorTask에서 주기적으로 호출해야 한다.
  */
void Buzzer_Update(void)
{
    uint32_t currTick = HAL_GetTick();

    switch (currentBuzzerState)
    {
        case BUZZER_IDLE:
            break;

        case BUZZER_HORN:
            /* 400 ms 동안 430/480 Hz를 교차하여 경적 음색을 만든다. */
            if ((currTick - hornStartTime) > 400U)
            {
                Buzzer_SetState(BUZZER_IDLE);
            }
            else if ((((currTick - hornStartTime) / 35U) % 2U) == 0U)
            {
                Buzzer_SetTone(430U);
            }
            else
            {
                Buzzer_SetTone(480U);
            }
            break;

        case BUZZER_REVERSE_BEEP:
            /* 후진 중 300 ms마다 ON/OFF를 반복한다. */
            if ((currTick - buzzerLastTick) >= 300U)
            {
                buzzerLastTick = currTick;

                if (beepToggle == 0U)
                {
                    Buzzer_On();
                    beepToggle = 1U;
                }
                else
                {
                    Buzzer_Off();
                    beepToggle = 0U;
                }
            }
            break;

        default:
            Buzzer_SetState(BUZZER_IDLE);
            break;
    }
}
