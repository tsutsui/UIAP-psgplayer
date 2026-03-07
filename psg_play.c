#include "ch32fun.h"

#define YM_PA_BC1       (1U << 1)   /* PA1 */
#define YM_PA_BDIR      (1U << 2)   /* PA2 */
#define YM_PA_CTRL_MASK (YM_PA_BC1 | YM_PA_BDIR)

#define YM_PC_DATA_MASK 0x00ffU     /* PC0..PC7 = DA0..DA7 */

#define YM_PD_RESET     (1U << 0)   /* PD0 */
#define YM_PD_CLK       (1U << 2)   /* PD2 = TIM1_CH1 */

static void
gpio_mask_write(volatile uint32_t *bshr, uint16_t mask, uint16_t value)
{
	uint32_t set_bits;
	uint32_t clr_bits;

	set_bits = value & mask;
	clr_bits = ((uint32_t)(mask & ~value)) << 16;
	*bshr = set_bits | clr_bits;
}

static void
ym_bus_hold(void)
{
	__asm__ volatile(
	    "nop\nnop\nnop\nnop\n"
	    "nop\nnop\nnop\nnop\n"
	    "nop\nnop\nnop\nnop\n"
	    "nop\nnop\nnop\nnop\n"
	    "nop\nnop\nnop\nnop\n"
	    "nop\nnop\nnop\nnop\n"
	    "nop\nnop\nnop\nnop\n"
	    "nop\nnop\nnop\nnop\n");
}

static void
ym_ctrl_inactive(void)
{
	gpio_mask_write(&GPIOA->BSHR, YM_PA_CTRL_MASK, 0);
}

static void
ym_ctrl_address(void)
{
	gpio_mask_write(&GPIOA->BSHR, YM_PA_CTRL_MASK,
	    YM_PA_BDIR | YM_PA_BC1);
}

static void
ym_ctrl_write(void)
{
	gpio_mask_write(&GPIOA->BSHR, YM_PA_CTRL_MASK, YM_PA_BDIR);
}

static void
ym_set_data(uint8_t v)
{
	gpio_mask_write(&GPIOC->BSHR, YM_PC_DATA_MASK, v);
}

static void
ym_reset_assert(void)
{
	gpio_mask_write(&GPIOD->BSHR, YM_PD_RESET, 0);
}

static void
ym_reset_deassert(void)
{
	gpio_mask_write(&GPIOD->BSHR, YM_PD_RESET, YM_PD_RESET);
}

static void
ym_latch_address(uint8_t reg)
{
	ym_ctrl_inactive();
	ym_set_data(reg & 0x0f);

	ym_ctrl_address();
	ym_bus_hold();

	ym_ctrl_inactive();
	ym_bus_hold();
}

static void
ym_write_data(uint8_t value)
{
	ym_ctrl_inactive();
	ym_set_data(value);

	ym_ctrl_write();
	ym_bus_hold();

	ym_ctrl_inactive();
	ym_bus_hold();
}

static void
ym_write_reg(uint8_t reg, uint8_t value)
{
	ym_latch_address(reg);
	ym_write_data(value);
}

static void
gpio_init_raw(void)
{
	RCC->APB2PCENR |= RCC_APB2Periph_GPIOA
	    | RCC_APB2Periph_GPIOC
	    | RCC_APB2Periph_GPIOD
	    | RCC_APB2Periph_AFIO
	    | RCC_APB2Periph_TIM1;

	/*
	 * PA1/PA2 を通常 GPIO として使う。
	 */
	AFIO->PCFR1 &= ~(1U << 15);

	/*
	 * PA1, PA2 = output push-pull 10MHz
	 */
	GPIOA->CFGLR &= ~((0x0fU << (4 * 1)) | (0x0fU << (4 * 2)));
	GPIOA->CFGLR |= ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 1))
	    | ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 2));

	/*
	 * PC0..PC7 = output push-pull 10MHz
	 */
	GPIOC->CFGLR = 0;
	GPIOC->CFGLR |=
	    ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 0)) |
	    ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 1)) |
	    ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 2)) |
	    ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 3)) |
	    ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 4)) |
	    ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 5)) |
	    ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 6)) |
	    ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 7));

	/*
	 * PD0 = RESET: output push-pull 10MHz
	 */
	GPIOD->CFGLR &= ~(0x0fU << (4 * 0));
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 0);

	/*
	 * PD2 = TIM1_CH1 出力
	 *
	 * CNF=10: Alternate function push-pull
	 * MODE=01: Output mode, max 10MHz
	 */
	GPIOD->CFGLR &= ~(0x0fU << (4 * 2));
	GPIOD->CFGLR |= ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF) << (4 * 2));

	ym_ctrl_inactive();
	ym_set_data(0x00);
	ym_reset_deassert();
}

static void
tim1_init_2mhz_clock(void)
{
	/*
	 * TIM1_CH1 on PD2
	 *
	 * 48MHz / (PSC+1) / (ARR+1)
	 * PSC=0, ARR=23 -> 2MHz
	 * CCR1=12 -> 約50% duty
	 */

	TIM1->PSC = 0;
	TIM1->ATRLR = 23;
	TIM1->CH1CVR = 12;

	/*
	 * PWM mode 1, preload enable
	 */
	TIM1->CHCTLR1 &= ~(
	    TIM_OC1M |
	    TIM_CC1S
	);
	TIM1->CHCTLR1 |=
	    TIM_OC1PE |
	    TIM_OC1M_2 | TIM_OC1M_1;

	/*
	 * CH1 enable
	 */
	TIM1->CCER |= TIM_CC1E;

	/*
	 * Advanced timer main output enable
	 */
	TIM1->BDTR |= TIM_MOE;

	/*
	 * Auto-reload preload enable
	 */
	TIM1->CTLR1 |= TIM_ARPE;

	/*
	 * Update generation to latch registers
	 */
	TIM1->SWEVGR |= TIM_UG;

	/*
	 * Counter enable
	 */
	TIM1->CTLR1 |= TIM_CEN;
}

static void
ym_chip_reset(void)
{
	ym_reset_assert();
	Delay_Ms(1);
	ym_reset_deassert();
	Delay_Ms(1);
}

static void
ym_all_off(void)
{
	ym_write_reg(7, 0x3f);
	ym_write_reg(8, 0x00);
	ym_write_reg(9, 0x00);
	ym_write_reg(10, 0x00);
}

int
main(void)
{
	/*
	 * 2MHz master clock 前提
	 *
	 * tone period = master / (16 * f)
	 *
	 * C4 ≒ 261.63Hz -> 478
	 * E4 ≒ 329.63Hz -> 379
	 * G4 ≒ 392.00Hz -> 319
	 */
	const uint16_t c4 = 478;
	const uint16_t e4 = 379;
	const uint16_t g4 = 319;

	SystemInit();
	gpio_init_raw();
	tim1_init_2mhz_clock();

	/*
	 * CLOCK が安定するのを少し待つ
	 */
	Delay_Ms(1);

	ym_chip_reset();
	ym_all_off();

	/* A = C4 */
	ym_write_reg(0, c4 & 0xff);
	ym_write_reg(1, (c4 >> 8) & 0x0f);

	/* B = E4 */
	ym_write_reg(2, e4 & 0xff);
	ym_write_reg(3, (e4 >> 8) & 0x0f);

	/* C = G4 */
	ym_write_reg(4, g4 & 0xff);
	ym_write_reg(5, (g4 >> 8) & 0x0f);

	/*
	 * R7:
	 *   bit0..2 = tone disable A/B/C
	 *   bit3..5 = noise disable A/B/C
	 *
	 * tone A/B/C on, noise A/B/C off
	 */
	ym_write_reg(7, 0x38);

	/* volume = 15 */
	ym_write_reg(8, 0x0f);
	ym_write_reg(9, 0x0f);
	ym_write_reg(10, 0x0f);

	Delay_Ms(5000);

	ym_all_off();

	while (1) {
	}
}