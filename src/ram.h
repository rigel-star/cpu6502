#ifndef RAM_H
#define RAM_H

#include "bytes.h"

#define MEM_MAX 0xFFFF

typedef struct ram
{
	byte data[MEM_MAX];
} ram_t;

void ram_init(ram_t *r);

byte ram_read(ram_t *ram, word addr);

void ram_write(ram_t *ram, word addr, byte data);

#endif
