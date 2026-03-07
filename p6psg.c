/*
 * p6psg.c
 *  PC-6001 PSGドライバ用演奏データ読み出し
 */

#include <string.h>

#include "p6psg.h"

typedef struct p6psg {
	const uint8_t *buf;
	size_t size;
} p6psg_t;

/* オブジェクト生成 */
p6psg_t *
p6psg_create(void)
{
	static p6psg_t psg;
	memset(&psg, 0, sizeof(psg));

	return &psg;
}

/* オブジェクト破棄 */
void
p6psg_destroy(p6psg_t *psg)
{
	return;
}

/* 演奏データオブジェクト読み込みおよびパース */
int
p6psg_load(p6psg_t *psg, const uint8_t *buf, int p6size, p6psg_channel_dataset_t *channels)
{
	p6psg_channel_dataset_t channels_tmp;

	if (psg == NULL)
		return 0;

	if (buf == NULL || channels == NULL) {
		return 0;
	}

	psg->size = 0;

	if (p6size < (8 + 3)) {
		goto fail;
	}

	if (p6size >= 0x10000) {
		goto fail;
	}

	/* parse and split P6 PSG data file per channel */
	uint16_t a_addr = ((uint16_t)buf[1] << 8) | buf[0];
	uint16_t b_addr = ((uint16_t)buf[3] << 8) | buf[2];
	uint16_t c_addr = ((uint16_t)buf[5] << 8) | buf[4];
	if (c_addr > p6size || b_addr >= c_addr || a_addr >= b_addr || a_addr < 8) {
		goto fail;
	}
	uint16_t a_size = b_addr - a_addr;
	uint16_t b_size = c_addr - b_addr;
	uint16_t c_size = p6size - c_addr;
	if (buf[a_addr + a_size - 1] != 0xff ||
	    buf[b_addr + b_size - 1] != 0xff ||
	    buf[c_addr + c_size - 1] != 0xff) {
		goto fail;
	}
	channels_tmp.ch[P6PSG_CH_A].ptr = &buf[a_addr];
	channels_tmp.ch[P6PSG_CH_A].len = a_size;
	channels_tmp.ch[P6PSG_CH_B].ptr = &buf[b_addr];
	channels_tmp.ch[P6PSG_CH_B].len = b_size;
	channels_tmp.ch[P6PSG_CH_C].ptr = &buf[c_addr];
	channels_tmp.ch[P6PSG_CH_C].len = c_size;

	psg->buf = buf;
	psg->size = p6size;
	*channels = channels_tmp;
	return 1;

 fail:
	psg->buf = NULL;
	psg->size = 0;

	return 0;
}
