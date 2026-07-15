/**
  ******************************************************************************
  * @file    ultrasonic.c
  * @brief   좌·중앙·우 초음파 센서 거리 측정
  * @details Trigger 10 us Pulse를 출력하고 Echo High 시간을 측정해
  *          거리로 변환한다. Echo 이상 상태에 대비해 Timeout을 적용한다.
  ******************************************************************************
  */

#include "ultrasonic.h"
#include "delay.h"

/* Echo를 받지 못했을 때 반환하는 오류 거리값 */
#define ULTRASONIC_NO_ECHO_CM    999U

/* Echo Rising Edge 대기 제한 시간 */
#define ECHO_START_TIMEOUT_US    6000U

/* Echo High Pulse 폭 측정 제한 시간 */
#define ECHO_HIGH_TIMEOUT_US     6000U

/**
  * @brief 하나의 초음파 센서 거리를 측정한다.
  * @param trig_port Trigger GPIO Port
  * @param trig_pin  Trigger GPIO Pin
  * @param echo_port Echo GPIO Port
  * @param echo_pin  Echo GPIO Pin
  * @return 측정 거리 [cm], 실패 시 ULTRASONIC_NO_ECHO_CM
  */
static uint16_t measure_distance(GPIO_TypeDef *trig_port,
                                 uint16_t trig_pin,
                                 GPIO_TypeDef *echo_port,
                                 uint16_t echo_pin)
{
    uint32_t echo_start_us;
    uint32_t echo_end_us;
    uint32_t wait_start_us;

    /* 센서 Trigger 입력을 안정적으로 Low 상태로 만든다. */
    HAL_GPIO_WritePin(trig_port, trig_pin, GPIO_PIN_RESET);
    delay_us(2);

    /* 10 us Trigger Pulse로 초음파 송신을 요청한다. */
    HAL_GPIO_WritePin(trig_port, trig_pin, GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(trig_port, trig_pin, GPIO_PIN_RESET);

    /* Echo Rising Edge를 기다린다. 센서 단선/무응답 시 Timeout 반환. */
    wait_start_us = delay_get_us();

    while (HAL_GPIO_ReadPin(echo_port, echo_pin) == GPIO_PIN_RESET)
    {
        if ((uint32_t)(delay_get_us() - wait_start_us) > ECHO_START_TIMEOUT_US)
        {
            return ULTRASONIC_NO_ECHO_CM;
        }
    }

    echo_start_us = delay_get_us();

    /* Echo High Pulse 종료를 기다린다. High 고착 시 Timeout 반환. */
    while (HAL_GPIO_ReadPin(echo_port, echo_pin) == GPIO_PIN_SET)
    {
        if ((uint32_t)(delay_get_us() - echo_start_us) > ECHO_HIGH_TIMEOUT_US)
        {
            return ULTRASONIC_NO_ECHO_CM;
        }
    }

    echo_end_us = delay_get_us();

    /*
     * 거리 = 왕복 시간 × 음속(0.034 cm/us) ÷ 2
     * 정수 연산식: pulse_us × 34 ÷ 2000
     */
    return (uint16_t)(((echo_end_us - echo_start_us) * 34U) / 2000U);
}

/** @brief 좌측 초음파 센서 거리 반환 [cm] */
uint16_t get_distance_left(void)
{
    return measure_distance(TRIG_L_GPIO_Port, TRIG_L_Pin,
                            ECHO_L_GPIO_Port, ECHO_L_Pin);
}

/** @brief 중앙 초음파 센서 거리 반환 [cm] */
uint16_t get_distance_center(void)
{
    return measure_distance(TRIG_C_GPIO_Port, TRIG_C_Pin,
                            ECHO_C_GPIO_Port, ECHO_C_Pin);
}

/** @brief 우측 초음파 센서 거리 반환 [cm] */
uint16_t get_distance_right(void)
{
    return measure_distance(TRIG_R_GPIO_Port, TRIG_R_Pin,
                            ECHO_R_GPIO_Port, ECHO_R_Pin);
}
