#include "ram.h"

void ram_init(ram_t *r)
{
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
