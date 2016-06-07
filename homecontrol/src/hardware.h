#ifndef _HARDWARE_H
#define _HARDWARE_H

#include "typedef.h"


int hardware_init(BOARD *board);
void hardware_close(BOARD *board);
int hardware_send(uint8_t *data, int size);

#ifndef OS_UBUNTU
void hardware_soft_version(uint8_t *soft_version);
#endif

#endif
