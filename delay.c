#include "delay.h"

static uint8_t delay_ready = 0;
static uint32_t last_cycle_count = 0;
static uint64_t cycle_high_word = 0;

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

void delay_us(uint16_t us)
{
    uint32_t start = delay_get_us();

    while ((uint32_t)(delay_get_us() - start) < us)
    {
        __NOP();
    }
}
