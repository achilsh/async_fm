#include "num2str.h"
#include <string.h>

namespace loss
{
    static const char lut[201] =
        "0001020304050607080910111213141516171819"
        "2021222324252627282930313233343536373839"
        "4041424344454647484950515253545556575859"
        "6061626364656667686970717273747576777879"
        "8081828384858687888990919293949596979899";

#define dd(u) ((const uint16_t)(lut[u]))
	static inline char* out2(const int d, char* p) 
	{
		memcpy(p, &((uint16_t *)lut)[d], 2);
		return p + 2;
	}

	static inline char* out1(const char in, char* p) 
	{
		memcpy(p, &in, 1);
		return p + 1;
	}

	static inline int digits( uint32_t u, unsigned k, int* d, char** p, int n ) 
	{
		if (u < k*10) {
			*d = u / k;
			*p = out1('0'+*d, *p);
			--n;
		}
		return n;
	}

	static inline char* itoa(uint32_t u, char* p, int d, int n) 
	{
		switch(n) 
		{
			case 10: d  = u / 100000000; p = out2( d, p );
			case  9: u -= d * 100000000;
			case  8: d  = u /   1000000; p = out2( d, p );
			case  7: u -= d *   1000000;
			case  6: d  = u /     10000; p = out2( d, p );
			case  5: u -= d *     10000;
			case  4: d  = u /       100; p = out2( d, p );
			case  3: u -= d *       100;
			case  2: d  = u /         1; p = out2( d, p );
			case  1: ;
		}
		*p = '\0';
		return p;
	}

    char* itoa_u32(uint32_t u, char* pDstBuf)
    {
		int d = 0,n;
		if (u >=100000000) n = digits(u, 100000000, &d, &pDstBuf, 10);
		else if (u <       100) n = digits(u,         1, &d, &pDstBuf,  2);
		else if (u <     10000) n = digits(u,       100, &d, &pDstBuf,  4);
		else if (u <   1000000) n = digits(u,     10000, &d, &pDstBuf,  6);
		else                    n = digits(u,   1000000, &d, &pDstBuf,  8);
		return itoa( u, pDstBuf, d, n );
    }
	//
    char *itoa_32(int32_t i, char* pDstBuf)
    {
		uint32_t u = i;
		if (i < 0) {
			*pDstBuf++ = '-';
			u = -u;
		}
		return itoa_u32(u, pDstBuf);
    }

    char *itoa_u64(uint64_t u, char *pDstBuf)
    {
		int d;

		uint32_t lower = (uint32_t)u;
		if (lower == u) return itoa_u32(lower, pDstBuf);

		uint64_t upper = u / 1000000000;
		pDstBuf = itoa_u64(upper, pDstBuf);
		lower = u - (upper * 1000000000);
		d = lower / 100000000;
		pDstBuf = out1('0'+d, pDstBuf);
		return itoa( lower, pDstBuf, d, 9 );
    }

    char *itoa_64(int64_t i, char *pDstbuf)
    {
		uint64_t u = i;
		if (i < 0) 
		{
			*pDstbuf++ = '-';
			u = -u;
		}
		return itoa_u64(u, pDstbuf);
    }
}
