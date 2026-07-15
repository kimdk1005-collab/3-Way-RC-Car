#ifndef INC_BLUETOOTH_H_
#define INC_BLUETOOTH_H_

#include "main.h"

// 스마트폰에서 들어오는 문자를 저장할 변수
extern uint8_t rx_byte;
extern volatile uint32_t bt_rx_drop_count;

// 블루투스 데이터 수신 함수 선언
void BT_Receive_Init(void);

#endif /* INC_BLUETOOTH_H_ */
