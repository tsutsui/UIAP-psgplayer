#include <stdint.h>
#include "ch32fun.h"
#include "ym2149_hw.h"

static void	app_start(void);
static void	app_stop(void);

static void
app_start(void)
{
	SystemInit();

	ym2149_hw_init();
	ym2149_hw_clock_start();

	/*
	 * CLOCK 安定待ち
	 */
	Delay_Ms(1);

	ym2149_hw_reset();
	ym2149_hw_stop();
}

static void
app_stop(void)
{

	ym2149_hw_stop();
	ym2149_hw_clock_stop();
	ym2149_hw_fini();
}

int
main(int argc, char *argv[])
{
	app_start();

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

	/* A = C4 */
	ym2149_hw_write_reg(0, c4 & 0xff);
	ym2149_hw_write_reg(1, (c4 >> 8) & 0x0f);

	/* B = E4 */
	ym2149_hw_write_reg(2, e4 & 0xff);
	ym2149_hw_write_reg(3, (e4 >> 8) & 0x0f);

	/* C = G4 */
	ym2149_hw_write_reg(4, g4 & 0xff);
	ym2149_hw_write_reg(5, (g4 >> 8) & 0x0f);

	/*
	 * R7:
	 *   bit0..2 = tone disable A/B/C
	 *   bit3..5 = noise disable A/B/C
	 *
	 * tone A/B/C on, noise A/B/C off
	 */
	ym2149_hw_write_reg(7, 0x38);

	/* volume = 15 */
	ym2149_hw_write_reg(8, 0x0f);
	ym2149_hw_write_reg(9, 0x0f);
	ym2149_hw_write_reg(10, 0x0f);

	Delay_Ms(5000);

	app_stop();

	while (1) {
	}
}