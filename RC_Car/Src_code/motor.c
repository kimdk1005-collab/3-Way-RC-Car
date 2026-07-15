/**
  ******************************************************************************
  * @file    motor.c
  * @brief   H-Bridge 방향 제어와 TIM2 차등 PWM 기반 모터 구동
  * @details 좌·우 모터의 방향 GPIO와 PWM Duty를 조합하여 직진, 후진,
  *          완만한 회전, 제자리 회전을 구현한다.
  ******************************************************************************
  */

#include "motor.h"

extern TIM_HandleTypeDef htim2;

/**
  * @brief TIM2 ARR 범위를 넘지 않도록 PWM 값을 제한한다.
  * @param speed 요청 PWM
  * @return 0~999 범위의 PWM
  */
static uint16_t clamp_speed(uint16_t speed)
{
    return (speed > 999U) ? 999U : speed;
}

/**
  * @brief 좌·우 모터를 동일 속도로 정방향 구동한다.
  * @param speed PWM Duty 값
  */
void motor_forward(uint16_t speed)
{
    speed = clamp_speed(speed);

    HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(IN3_GPIO_Port, IN3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IN4_GPIO_Port, IN4_Pin, GPIO_PIN_RESET);

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, speed);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, speed);
}

/**
  * @brief 좌·우 모터 PWM을 독립 설정해 차등 조향한다.
  * @param left_speed  좌측 모터 PWM
  * @param right_speed 우측 모터 PWM
  */
void motor_forward_diff(uint16_t left_speed,
                        uint16_t right_speed)
{
    left_speed  = clamp_speed(left_speed);
    right_speed = clamp_speed(right_speed);

    HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(IN3_GPIO_Port, IN3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IN4_GPIO_Port, IN4_Pin, GPIO_PIN_RESET);

    __HAL_TIM_SET_COMPARE(&htim2,
                          TIM_CHANNEL_1,
                          left_speed);

    __HAL_TIM_SET_COMPARE(&htim2,
                          TIM_CHANNEL_2,
                          right_speed);
}

/**
  * @brief 좌·우 모터를 동일 속도로 역방향 구동한다.
  * @param speed PWM Duty 값
  */
void motor_backward(uint16_t speed)
{
    speed = clamp_speed(speed);

    HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(IN3_GPIO_Port, IN3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN4_GPIO_Port, IN4_Pin, GPIO_PIN_SET);

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, speed);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, speed);
}

/**
  * @brief 모터 방향 입력과 PWM을 모두 0으로 만들어 정지한다.
  */
void motor_stop(void)
{
    HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(IN3_GPIO_Port, IN3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN4_GPIO_Port, IN4_Pin, GPIO_PIN_RESET);

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
}

/**
  * @brief 우측 모터 속도를 낮춰 완만하게 우회전한다.
  * @param speed 외측인 좌측 모터의 기준 PWM
  */
void motor_turn_right(uint16_t speed)
{
    uint16_t left_pwm  = speed;
    uint16_t right_pwm = speed / 3;

    left_pwm  = clamp_speed(left_pwm);
    right_pwm = clamp_speed(right_pwm);

    HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(IN3_GPIO_Port, IN3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IN4_GPIO_Port, IN4_Pin, GPIO_PIN_RESET);

    __HAL_TIM_SET_COMPARE(&htim2,
                          TIM_CHANNEL_1,
                          left_pwm);

    __HAL_TIM_SET_COMPARE(&htim2,
                          TIM_CHANNEL_2,
                          right_pwm);
}

/**
  * @brief 좌측 모터 속도를 낮춰 완만하게 좌회전한다.
  * @param speed 외측인 우측 모터의 기준 PWM
  */
void motor_turn_left(uint16_t speed)
{
    uint16_t left_pwm  = speed / 3;
    uint16_t right_pwm = speed;

    left_pwm  = clamp_speed(left_pwm);
    right_pwm = clamp_speed(right_pwm);

    HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(IN3_GPIO_Port, IN3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IN4_GPIO_Port, IN4_Pin, GPIO_PIN_RESET);

    __HAL_TIM_SET_COMPARE(&htim2,
                          TIM_CHANNEL_1,
                          left_pwm);

    __HAL_TIM_SET_COMPARE(&htim2,
                          TIM_CHANNEL_2,
                          right_pwm);
}

/**
  * @brief 좌측은 정방향, 우측은 역방향으로 구동해 제자리 우회전한다.
  * @param speed 좌·우 모터 PWM
  */
void motor_spin_right(uint16_t speed)
{
    speed = clamp_speed(speed);

    HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(IN3_GPIO_Port, IN3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN4_GPIO_Port, IN4_Pin, GPIO_PIN_SET);

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, speed);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, speed);
}

/**
  * @brief 좌측은 역방향, 우측은 정방향으로 구동해 제자리 좌회전한다.
  * @param speed 좌·우 모터 PWM
  */
void motor_spin_left(uint16_t speed)
{
    speed = clamp_speed(speed);

    HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(IN3_GPIO_Port, IN3_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(IN4_GPIO_Port, IN4_Pin, GPIO_PIN_RESET);

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, speed);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, speed);
}
