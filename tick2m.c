/*
 * tick2m.c
 *  TIM2 による 2ms タイマ割り込み処理
 */

#include "ch32fun.h"
#include "tick2m.h"

static volatile uint32_t g_tick2m_pending;

void TIM2_IRQHandler(void) __attribute__((interrupt));

void
tick2m_init(void)
{
	RCC->APB1PCENR |= RCC_APB1Periph_TIM2;

	/*
	 * 48MHz / (47 + 1) = 1MHz
	 * 1MHz / (1999 + 1) = 500Hz = 2ms
	 *
	 * PC-6001 の 2ms 割り込みは正確には 2.05128192 ms
	 * (3.9936MHz の 4分周クロックの 2048カウント) なので
	 * 1MHz / (2050 + 1) の 2.051ms に設定する
	 */
	TIM2->CTLR1 = 0;
	TIM2->PSC = 47;
	TIM2->ATRLR = 2050;
	TIM2->CNT = 0;
	TIM2->INTFR = 0;
	TIM2->DMAINTENR |= TIM_UIE;
	TIM2->SWEVGR |= TIM_UG;

	g_tick2m_pending = 0;
}

void
tick2m_fini(void)
{
	tick2m_stop();

	TIM2->DMAINTENR &= ~TIM_UIE;
	TIM2->INTFR = 0;
	TIM2->CNT = 0;
	TIM2->PSC = 0;
	TIM2->ATRLR = 0;

	NVIC_DisableIRQ(TIM2_IRQn);
	RCC->APB1PCENR &= ~RCC_APB1Periph_TIM2;

	g_tick2m_pending = 0;
}

void
tick2m_start(void)
{
	g_tick2m_pending = 0;
	TIM2->INTFR = 0;
	NVIC_EnableIRQ(TIM2_IRQn);
	TIM2->CTLR1 |= TIM_CEN;
}

void
tick2m_stop(void)
{
	TIM2->CTLR1 &= ~TIM_CEN;
	NVIC_DisableIRQ(TIM2_IRQn);
	TIM2->INTFR = 0;
}

uint32_t
tick2m_take_pending(void)
{
	uint32_t pending;

	__disable_irq();
	pending = g_tick2m_pending;
	g_tick2m_pending = 0;
	__enable_irq();

	return pending;
}

void
TIM2_IRQHandler(void)
{
	if ((TIM2->INTFR & TIM_UIF) == 0) {
		return;
	}

	TIM2->INTFR &= ~TIM_UIF;
	g_tick2m_pending++;
}
