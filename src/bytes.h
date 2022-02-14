#ifndef BYTES_H
#define BYTES_H

#include <stdint.h>

typedef uint8_t byte;
typedef uint16_t word;

inline word swap_bytes_w(word n)
{
	return (((n & 0xFF00) >> 8) | ((n & 0x00FF) << 8));
}

inline word big_endian_w(word w)
{
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	return w;
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return swap_bytes_w(w);
#else
	#error endianess not supported
#endif
	return 0x0;
}

inline word little_endian_w(word w)
{
#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	return swap_bytes_w(w);
#elif __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return w;
#else
	#error endianess not supported
#endif
	return 0x0;
}

#endif
