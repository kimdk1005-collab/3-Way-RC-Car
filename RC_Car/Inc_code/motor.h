#ifndef INC_MOTOR_H_
#define INC_MOTOR_H_

#include "main.h"

void motor_forward(uint16_t speed);
void motor_backward(uint16_t speed);
void motor_stop(void);

void motor_turn_right(uint16_t speed);
void motor_turn_left(uint16_t speed);

void motor_spin_right(uint16_t speed);
void motor_spin_left(uint16_t speed);

void motor_forward_diff(uint16_t left_speed,
                        uint16_t right_speed);

#endif
