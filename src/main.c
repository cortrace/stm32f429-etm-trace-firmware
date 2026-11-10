/* STM32F429I-DISC1 bare-metal trace workload.
 *
 * Runs at 60 MHz from the 8 MHz HSE and loops over a small call tree (fib,
 * crunch, branchy, leaf) followed by a long straight-line spin, so that the
 * trace shows real calls with a low average data rate. PG13 (green LED) toggles
 * once per round. The trace pins (PE2..PE6), DBGMCU_CR and the CoreSight blocks
 * are set up by the host (`cortrace arm --platform stm32f429`), not here.
 */
#include "stm32f4xx.h"

volatile uint32_t sink;
volatile uint32_t rounds;

__attribute__((noinline)) uint32_t leaf(uint32_t x) { return x * 2654435761u + 1u; }

__attribute__((noinline)) uint32_t fib(uint32_t n) { return n < 2 ? n : fib(n - 1) + fib(n - 2); }

__attribute__((noinline)) uint32_t crunch(uint32_t n)
{
    uint32_t acc = 0;
    for (uint32_t i = 0; i < n; i++)
        acc ^= leaf(i);
    return acc;
}

__attribute__((noinline)) uint32_t branchy(uint32_t n)
{
    uint32_t acc = 0;
    for (uint32_t i = 0; i < n; i++) {
        if (i & 1)
            acc += i;
        else
            acc -= i >> 1;
        if ((i % 7) == 3)
            acc ^= 0x55;
    }
    return acc;
}

/* 255 dependent adds per loop: many cycles, one branch each */
__attribute__((noinline)) void spin(uint32_t n)
{
    uint32_t x = n;
    do {
        __asm volatile(".rept 255\n adds %0, %0, #1\n .endr\n" : "+r"(x));
        n--;
    } while (n != 0);
    sink = x;
}

__attribute__((noinline)) void work(void)
{
    sink = fib(12);
    sink = crunch(200);
    sink = branchy(300);
}

static void clock_60mhz(void)
{
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) {
    }
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    PWR->CR |= PWR_CR_VOS; /* scale 1 */
    FLASH->ACR = FLASH_ACR_LATENCY_2WS | FLASH_ACR_ICEN | FLASH_ACR_DCEN | FLASH_ACR_PRFTEN;
    /* VCO = 8 MHz / 8 * 240 = 240 MHz; SYSCLK = /4 = 60 MHz; USB = /5.
     * TRACECLK = HCLK/2 = 30 MHz: the bare board has no series resistors on the
     * trace pins, so keep the parallel trace clock well below 80 MHz. */
    RCC->PLLCFGR = (8u << RCC_PLLCFGR_PLLM_Pos) | (240u << RCC_PLLCFGR_PLLN_Pos) | (1u << RCC_PLLCFGR_PLLP_Pos) |
                   RCC_PLLCFGR_PLLSRC_HSE | (5u << RCC_PLLCFGR_PLLQ_Pos);
    RCC->CFGR = RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV2 | RCC_CFGR_PPRE2_DIV1;
    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {
    }
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {
    }
}

int main(void)
{
    clock_60mhz();
    SystemCoreClockUpdate();
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOGEN;
    GPIOG->MODER = (GPIOG->MODER & ~(3u << 26)) | (1u << 26); /* PG13 output */
    for (;;) {
        work();
        spin(200);
        GPIOG->ODR ^= 1u << 13;
        rounds++;
    }
}
