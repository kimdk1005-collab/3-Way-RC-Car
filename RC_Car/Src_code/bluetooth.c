/**
  ******************************************************************************
  * @file    bluetooth.c
  * @brief   스마트폰과 전용 컨트롤러의 UART 수신 인터럽트 처리
  * @details 수신 ISR에서는 명령을 해석하지 않고 FreeRTOS Message Queue에
  *          전달하여 인터럽트 실행 시간을 최소화한다.
  ******************************************************************************
  */

#include "bluetooth.h"
#include "usart.h"
#include "cmsis_os.h"

/* USART1: 스마트폰 Bluetooth 수신 버퍼 */
uint8_t rx_byte;

/* USART6: 전용 컨트롤러 Bluetooth 수신 버퍼 */
uint8_t ctrl_rx_byte;

/* Queue가 가득 차 명령을 저장하지 못한 횟수. 디버깅 지표로 사용한다. */
volatile uint32_t bt_rx_drop_count = 0;

/* freertos.c에서 생성되는 공용 명령 Queue */
extern osMessageQueueId_t btQueueHandle;

/**
  * @brief 두 UART 채널의 1-byte 인터럽트 수신을 시작한다.
  * @note  각 수신 완료 콜백에서 다음 1-byte 수신을 다시 등록한다.
  */
void BT_Receive_Init(void)
{
    HAL_UART_Receive_IT(&huart1, &rx_byte, 1);       /* Smartphone */
    HAL_UART_Receive_IT(&huart6, &ctrl_rx_byte, 1);  /* Physical controller */
}

/**
  * @brief UART 1-byte 수신 완료 콜백
  * @param huart 수신 이벤트가 발생한 UART Handle
  * @note  ISR 문맥이므로 대기 시간 0으로 Queue에 저장한다.
  */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        /* 스마트폰 명령을 공용 Queue에 전달한다. */
        if (btQueueHandle != NULL)
        {
            if (osMessageQueuePut(btQueueHandle, &rx_byte, 0, 0) != osOK)
            {
                bt_rx_drop_count++;
            }
        }

        /* 다음 스마트폰 명령 수신을 즉시 재등록한다. */
        HAL_UART_Receive_IT(&huart1, &rx_byte, 1);
    }
    else if (huart->Instance == USART6)
    {
        /* 전용 컨트롤러 명령도 동일한 Queue로 전달한다. */
        if (btQueueHandle != NULL)
        {
            if (osMessageQueuePut(btQueueHandle, &ctrl_rx_byte, 0, 0) != osOK)
            {
                bt_rx_drop_count++;
            }
        }

        /* 다음 컨트롤러 명령 수신을 즉시 재등록한다. */
        HAL_UART_Receive_IT(&huart6, &ctrl_rx_byte, 1);
    }
}
