#ifndef INC_DELAY_H_
#define INC_DELAY_H_

#include "main.h"

void delay_init(void);
void delay_us(uint16_t us);
uint32_t delay_get_us(void);

#endif /* INC_DELAY_H_ */
