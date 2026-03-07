/*
 * UIAPduino で PSG YM2149F を鳴らすデモ YM2149F HW依存層
 * 接続は以下
 * D2/PC0:     DA0
 * D3/PC1:     DA1
 * D4/PC2:     DA2
 * D5/PC3:     DA3
 * A2/D6/PC4:  DA4
 * D7/PC5:     DA5
 * D8/PC6:     DA6
 * D9/PC7:     DA7
 * A1/D0/PA1:  BC1
 * A0/D1/PA2:  BDIR
 * D10/PD0:    RESET (直結でlow-active; Raspberry Pi GPIO版とは異なる)
 * A3/D12/PD2: 2MHz CLOCK (TIM1 PWM)
 *  YM2149Fの BC2は回路上で H 固定
 */

#include "ch32fun.h"
#include "ym2149_hw.h"

#define	YM_PA_BC1		(1U << 1)	/* PA1 */
#define	YM_PA_BDIR		(1U << 2)	/* PA2 */
#define	YM_PA_CTRL_MASK		(YM_PA_BC1 | YM_PA_BDIR)

#define	YM_PC_DATA_MASK		0x00ffU		/* PC0..PC7 */

#define	YM_PD_RESET		(1U << 0)	/* PD0 */
#define	YM_PD_CLK		(1U << 2)	/* PD2 = TIM1_CH1 */

static void	gpio_mask_write(volatile uint32_t *, uint16_t, uint16_t);
static void	ym_bus_hold(void);
static void	ym_ctrl_inactive(void);
static void	ym_ctrl_address(void);
static void	ym_ctrl_write(void);
static void	ym_set_data(uint8_t);
static void	ym2149_hw_reset_assert(void);
static void	ym2149_hw_reset_deassert(void);
static void	ym_pd2_set_gpio_low(void);
static void	ym_pd2_set_tim1_ch1(void);

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
	/*
	 * YM2149 write/address hold time を十分満たすため、
	 * 少しだけ待つ。
	 * 48MHz で 32 NOP は約 0.67us。
	 */
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
ym2149_hw_reset_assert(void)
{
	gpio_mask_write(&GPIOD->BSHR, YM_PD_RESET, 0);
}

static void
ym2149_hw_reset_deassert(void)
{
	gpio_mask_write(&GPIOD->BSHR, YM_PD_RESET, YM_PD_RESET);
}

/* PD2 を GPIOに戻す */
static void
ym_pd2_set_gpio_low(void)
{
	GPIOD->CFGLR &= ~(0x0fU << (4 * 2));
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 2);
	gpio_mask_write(&GPIOD->BSHR, YM_PD_CLK, 0);
}

/* PD2 を PWM 出力に設定 */
static void
ym_pd2_set_tim1_ch1(void)
{
	GPIOD->CFGLR &= ~(0x0fU << (4 * 2));
	GPIOD->CFGLR |=
	    (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP_AF) << (4 * 2);
}

void
ym2149_hw_init(void)
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
	 * PA1, PA2 = BC1, BDIR
	 */
	GPIOA->CFGLR &= ~((0x0fU << (4 * 1)) | (0x0fU << (4 * 2)));
	GPIOA->CFGLR |= ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 1))
	    | ((GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 2));

	/*
	 * PC0..PC7 = DA0..DA7
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
	 * PD0 = RESET
	 */
	GPIOD->CFGLR &= ~(0x0fU << (4 * 0));
	GPIOD->CFGLR |= (GPIO_Speed_10MHz | GPIO_CNF_OUT_PP) << (4 * 0);

	/*
	 * PD2 = CLOCK output pin
	 * 初期状態では GPIO low にしておく。
	 */
	ym_pd2_set_gpio_low();

	ym_ctrl_inactive();
	ym_set_data(0x00);
	ym2149_hw_reset_deassert();
}

void
ym2149_hw_fini(void)
{
	ym2149_hw_clock_stop();
	ym2149_hw_stop();
	ym_ctrl_inactive();
	ym_set_data(0x00);

	RCC->APB2PCENR &= ~(RCC_APB2Periph_TIM1
	    | RCC_APB2Periph_GPIOA
	    | RCC_APB2Periph_GPIOC
	    | RCC_APB2Periph_GPIOD
	    | RCC_APB2Periph_AFIO);
}

void
ym2149_hw_clock_start(void)
{
	/*
	 * TIM1_CH1 on PD2
	 *
	 * 48MHz / (PSC + 1) / (ATRLR + 1)
	 * PSC = 0, ATRLR = 23 -> 2MHz
	 * CH1CVR = 12 -> 約 50% duty
	 */
	ym_pd2_set_tim1_ch1();

	TIM1->CTLR1 = 0;
	TIM1->PSC = 0;
	TIM1->ATRLR = 23;
	TIM1->CH1CVR = 12;

	TIM1->CHCTLR1 &= ~(TIM_OC1M | TIM_CC1S);
	TIM1->CHCTLR1 |= TIM_OC1PE | TIM_OC1M_2 | TIM_OC1M_1;

	TIM1->CCER &= ~TIM_CC1E;
	TIM1->CCER |= TIM_CC1E;

	TIM1->BDTR |= TIM_MOE;
	TIM1->CTLR1 |= TIM_ARPE;
	TIM1->SWEVGR |= TIM_UG;
	TIM1->CTLR1 |= TIM_CEN;
}

void
ym2149_hw_clock_stop(void)
{
	TIM1->CTLR1 &= ~TIM_CEN;
	TIM1->CCER &= ~TIM_CC1E;
	TIM1->BDTR &= ~TIM_MOE;
	TIM1->SWEVGR |= TIM_UG;

	ym_pd2_set_gpio_low();
}

void
ym2149_hw_reset(void)
{
	ym2149_hw_reset_assert();
	Delay_Ms(1);
	ym2149_hw_reset_deassert();
	Delay_Ms(1);
}

void
ym2149_hw_write_reg(uint8_t reg, uint8_t value)
{
	ym_ctrl_inactive();
	ym_set_data(reg & 0x0f);

	ym_ctrl_address();
	ym_bus_hold();

	ym_ctrl_inactive();
	ym_bus_hold();

	ym_set_data(value);

	ym_ctrl_write();
	ym_bus_hold();

	ym_ctrl_inactive();
	ym_bus_hold();
}

void
ym2149_hw_stop(void)
{
	ym2149_hw_write_reg(7, 0x3f);
	ym2149_hw_write_reg(8, 0x00);
	ym2149_hw_write_reg(9, 0x00);
	ym2149_hw_write_reg(10, 0x00);
}