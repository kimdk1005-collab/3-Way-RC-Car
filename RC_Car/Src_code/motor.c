#include "motor.h"

extern TIM_HandleTypeDef htim2;

static uint16_t clamp_speed(uint16_t speed)
{
    return (speed > 999U) ? 999U : speed;
}

/************************************************
 * 직진
 ************************************************/
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

/************************************************
 * 차등 직진 (신규)
 ************************************************/
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

/************************************************
 * 후진
 ************************************************/
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

/************************************************
 * 정지
 ************************************************/
void motor_stop(void)
{
    HAL_GPIO_WritePin(IN1_GPIO_Port, IN1_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN2_GPIO_Port, IN2_Pin, GPIO_PIN_RESET);

    HAL_GPIO_WritePin(IN3_GPIO_Port, IN3_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IN4_GPIO_Port, IN4_Pin, GPIO_PIN_RESET);

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, 0);
    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_2, 0);
}

/************************************************
 * 완만한 우회전
 ************************************************/
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

/************************************************
 * 완만한 좌회전
 ************************************************/
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

/************************************************
 * 제자리 우회전
 ************************************************/
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

/************************************************
 * 제자리 좌회전
 ************************************************/
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
