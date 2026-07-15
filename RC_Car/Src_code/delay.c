/**
  ******************************************************************************
  * @file    delay.c
  * @brief   Cortex-M4 DWT Cycle Counter 기반 마이크로초 시간 함수
  * @details 초음파 Trigger Pulse와 Echo Pulse 폭 측정에 사용한다.
  ******************************************************************************
  */

#include "delay.h"

/* DWT 초기화 중복 실행 방지 */
static uint8_t delay_ready = 0;

/* 32-bit CYCCNT Overflow 검출용 직전 값 */
static uint32_t last_cycle_count = 0;

/* CYCCNT Overflow 누적 상위 워드 */
static uint64_t cycle_high_word = 0;

/**
  * @brief DWT Cycle Counter를 활성화한다.
  * @note  이미 초기화된 경우 아무 동작도 하지 않는다.
  */
void delay_init(void)
{
    if (delay_ready != 0U)
    {
        return;
    }

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    last_cycle_count = 0U;
    cycle_high_word = 0U;
    delay_ready = 1U;
}

/**
  * @brief 시스템 시작 이후 경과 시간을 마이크로초 단위로 반환한다.
  * @return 경과 시간 [us]
  * @note  32-bit CYCCNT Overflow를 소프트웨어로 확장해 연속 시간을 만든다.
  */
uint32_t delay_get_us(void)
{
    uint32_t cycle_count;
    uint64_t total_cycles;

    delay_init();

    cycle_count = DWT->CYCCNT;

    if (cycle_count < last_cycle_count)
    {
        cycle_high_word += 0x100000000ULL;
    }

    last_cycle_count = cycle_count;
    total_cycles = cycle_high_word + cycle_count;

    return (uint32_t)(total_cycles / (SystemCoreClock / 1000000U));
}

/**
  * @brief 지정한 시간만큼 Busy-wait한다.
  * @param us 대기 시간 [us]
  * @warning 짧은 Trigger Pulse 생성용이며 긴 지연에는 사용하지 않는다.
  */
void delay_us(uint16_t us)
{
    uint32_t start = delay_get_us();

    while ((uint32_t)(delay_get_us() - start) < us)
    {
        __NOP();
    }
}
