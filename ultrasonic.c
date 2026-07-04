#include "ultrasonic.h"
#include "delay.h"

#define ULTRASONIC_NO_ECHO_CM 999U
#define ECHO_START_TIMEOUT_US 6000U
#define ECHO_HIGH_TIMEOUT_US  6000U

static uint16_t measure_distance(GPIO_TypeDef *trig_port,
                                 uint16_t trig_pin,
                                 GPIO_TypeDef *echo_port,
                                 uint16_t echo_pin)
{
  uint32_t echo_start_us;
  uint32_t echo_end_us;
  uint32_t wait_start_us;

  HAL_GPIO_WritePin(trig_port, trig_pin, GPIO_PIN_RESET);
  delay_us(2);
  HAL_GPIO_WritePin(trig_port, trig_pin, GPIO_PIN_SET);
  delay_us(10);
  HAL_GPIO_WritePin(trig_port, trig_pin, GPIO_PIN_RESET);

  wait_start_us = delay_get_us();
  while (HAL_GPIO_ReadPin(echo_port, echo_pin) == GPIO_PIN_RESET)
  {
    if ((uint32_t)(delay_get_us() - wait_start_us) > ECHO_START_TIMEOUT_US)
    {
      return ULTRASONIC_NO_ECHO_CM;
    }
  }

  echo_start_us = delay_get_us();
  while (HAL_GPIO_ReadPin(echo_port, echo_pin) == GPIO_PIN_SET)
  {
    if ((uint32_t)(delay_get_us() - echo_start_us) > ECHO_HIGH_TIMEOUT_US)
    {
      return ULTRASONIC_NO_ECHO_CM;
    }
  }
  echo_end_us = delay_get_us();

  return (uint16_t)(((echo_end_us - echo_start_us) * 34U) / 2000U);
}

uint16_t get_distance_left(void)
{
  return measure_distance(TRIG_L_GPIO_Port, TRIG_L_Pin, ECHO_L_GPIO_Port, ECHO_L_Pin);
}

uint16_t get_distance_center(void)
{
  return measure_distance(TRIG_C_GPIO_Port, TRIG_C_Pin, ECHO_C_GPIO_Port, ECHO_C_Pin);
}

uint16_t get_distance_right(void)
{
  return measure_distance(TRIG_R_GPIO_Port, TRIG_R_Pin, ECHO_R_GPIO_Port, ECHO_R_Pin);
}
