#ifndef YM2149_HW_H
#define YM2149_HW_H

#include <stdint.h>

void	ym2149_hw_init(void);
void	ym2149_hw_fini(void);

void	ym2149_hw_clock_start(void);
void	ym2149_hw_clock_stop(void);

void	ym2149_hw_reset(void);

void	ym2149_hw_write_reg(uint8_t, uint8_t);
void	ym2149_hw_stop(void);

#endif /* YM2149_HW_H */