#ifndef TICK2M_H
#define TICK2M_H

#include <stdint.h>

void		tick2m_init(void);
void		tick2m_fini(void);

void		tick2m_start(void);
void		tick2m_stop(void);

uint32_t	tick2m_take_pending(void);

#endif /* TICK2M_H */