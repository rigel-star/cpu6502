#include "ef.h"

#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <string.h>

ef_file read_ef(const char *effname)
{
	int fd = open(effname, O_RDONLY);
	if(fd < 0)
	{
		fprintf(stderr, "File not found or permission denied: %s\n", effname);
		exit(1);
	}

	struct stat statbuf;
	if(fstat(fd, &statbuf) < 0)
	{
		fprintf(stderr, "Error getting information of: %s\n", effname);
		exit(2);
	}

	const size_t LEN = statbuf.st_size;
	char *mapped_file = mmap(NULL, LEN, PROT_READ, MAP_SHARED, fd, 0); 

	if(mapped_file == MAP_FAILED)
	{
		fprintf(stderr, "mmap failed\n");
		exit(4);
	}
	
	ef_file hdr;
	hdr.ef_magic[0] = *(mapped_file);
	hdr.ef_magic[1] = *(mapped_file + 1);
	hdr.ef_size = (word) *(mapped_file + 2);
	hdr.ef_data = (byte*) strdup(mapped_file + 4);

	munmap(mapped_file, LEN);
	return hdr;
}

void free_ef(ef_file *hdr)
{
	free(hdr->ef_data);
}
