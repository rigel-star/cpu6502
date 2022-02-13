#ifndef EF_H
#define EF_H

#include "bytes.h"

typedef struct // __attribute__((packed))
{
	byte 		ef_magic[2];
	word 		ef_size;
	byte		*ef_data;
} ef_file;

ef_file read_ef(const char *effname);
void free_ef(ef_file *hdr);

#endif
