#include "ram.h"
#include <stdlib.h>

void ram_init(ram_t *r)
{
	r->data = malloc(MEM_MAX * sizeof(byte));
	for(word i = 0; i < MEM_MAX; i++)
		r->data[i] = 0x0;
}

byte ram_read(ram_t *ram, word addr)
{
	if(addr >= 0 && addr <= MEM_MAX)
		return ram->data[addr];
	return 0;
}

void ram_write(ram_t *ram, word addr, byte data)
{
	if(addr >= 0 && addr <= MEM_MAX)
		ram->data[addr] = data;
}

void ram_free(ram_t *ram)
{
	free(ram->data);
}
