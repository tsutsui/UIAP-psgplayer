/*
 * psg_play.c
 *  Minimal YM2149 (AY-3-8910 compatible) player
 *  UIAPduino version
 */

#include <stdint.h>
#include "ch32fun.h"
#include "ym2149_hw.h"
#include "tick2m.h"
#include "psg_driver.h"
#include "p6psg.h"

/* コンパイル済み P6 PSG曲データ */
#include "psg_data.c"

static p6psg_t *psg;
static p6psg_channel_dataset_t channels;
static PSGDriver psgdriver;
static PSGDriver *drv = &psgdriver;

static void	app_start(void);
static void	app_stop(void);
static void	psg_write_reg_cb(void *, uint8_t, uint8_t);

static void
psg_write_reg_cb(void *opaque, uint8_t reg, uint8_t value)
{

	(void)opaque;
	ym2149_hw_write_reg(reg, value);
}

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

	/*
	 * 演奏データ準備とP6 PSG演奏ドライバ初期化と
	 */
	psg = p6psg_create();
	p6psg_load(psg, psgdata, sizeof(psgdata), &channels);
	psg_driver_init(drv, psg_write_reg_cb, NULL, NULL);
	psg_driver_set_channel_data(drv, P6PSG_CH_A, channels.ch[P6PSG_CH_A].ptr);
	psg_driver_set_channel_data(drv, P6PSG_CH_B, channels.ch[P6PSG_CH_B].ptr);
	psg_driver_set_channel_data(drv, P6PSG_CH_C, channels.ch[P6PSG_CH_C].ptr);
	psg_driver_start(drv);

	/*
	 * 2ms割り込み開始
	 */
	tick2m_init();
	tick2m_start();
}

static void
app_stop(void)
{
	/*
	 * 2ms割り込み停止
	 */
	tick2m_stop();
	tick2m_fini();

	/*
	 * 演奏ドライバ停止
	 */
	psg_driver_stop(drv);
	p6psg_destroy(psg);

	ym2149_hw_stop();
	ym2149_hw_clock_stop();
	ym2149_hw_fini();
}

int
main(int argc, char *argv[])
{
	uint32_t pending;

	app_start();

	for (;;) {
		pending = tick2m_take_pending();

		while (pending != 0) {
			pending--;

			/*
			 * PSGドライバ演奏周期処理呼び出し
			 */
			psg_driver_tick(drv);
		}
	}

	app_stop();

	return 0;
}
