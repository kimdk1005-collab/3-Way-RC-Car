#include "bluetooth.h"
#include "usart.h"
#include "cmsis_os.h" // FreeRTOS 우체통을 쓰기 위해 추가

uint8_t rx_byte;        // 스마트폰(USART1) 임시 수신 변수
uint8_t ctrl_rx_byte;   // 물리 컨트롤러(USART2) 임시 수신 변수 ⭐ 추가

volatile uint32_t bt_rx_drop_count = 0;
extern osMessageQueueId_t btQueueHandle; // freertos.c에 있는 우체통 가져오기

void BT_Receive_Init(void) {
    // 1. 스마트폰(USART1) 1바이트 수신 대기
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);

    // 2. 물리 컨트롤러(USART2) 1바이트 수신 대기 ⭐ 추가
    HAL_UART_Receive_IT(&huart6, &ctrl_rx_byte, 1);
}

// 통합 수신 완료 인터럽트 (스마트폰이나 컨트롤러에서 신호가 올 때마다 자동으로 실행됨)
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {

    // [경로 1] 스마트폰(USART1)에서 신호가 온 경우
    if (huart->Instance == USART1) {
        // 수신된 문자를 우체통에 넣음
        if (btQueueHandle != NULL) {
            if (osMessageQueuePut(btQueueHandle, &rx_byte, 0, 0) != osOK) {
                bt_rx_drop_count++;
            }
        }
        // 다시 다음 스마트폰 명령 수신 대기
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }

    // [경로 2] 물리 컨트롤러(USART2)에서 신호가 온 경우 ⭐ 추가
    else if (huart->Instance == USART6) {
        // 컨트롤러의 신호도 똑같은 우체통에 골인!
        // 이렇게 하면 모터 제어 담당 Task가 알아서 척척 처리합니다.
        if (btQueueHandle != NULL) {
            if (osMessageQueuePut(btQueueHandle, &ctrl_rx_byte, 0, 0) != osOK) {
                bt_rx_drop_count++;
            }
        }
        // 다시 다음 컨트롤러 명령 수신 대기
        HAL_UART_Receive_IT(&huart6, &ctrl_rx_byte, 1);
    }
}
