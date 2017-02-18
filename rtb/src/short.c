
#include <stdint.h> /* uintXX_t */
#include <string.h> /* memcpy, memset */

static inline uint64_t Rot64(uint64_t x, int k)
{
	return (x << k) | (x >> (64 - k));
}

#define INIT_VAL 0xDEADBEEFDEADBEEFLL

#if defined(SOFT)

/* software version */

#define EMIX(y,x,r) y = (y ^ x) + (x = Rot64(x,r));

void short128_hash(const void *key, int len, int seed, void *out) {
	uint64_t a, b, c, d;
	union {
		const uint8_t *p8; 
		const uint32_t *p32;
		const uint64_t *p64; 
	} u;

	u.p8 = (const uint8_t *)key;
	a = b = seed;
	c = d = INIT_VAL;

	while (len > 0) {
		a ^=  (uint64_t)len;
		b ^= ~(uint64_t)len;
		if (len < 16) {
			switch (len) {
			case 15:
				d += ((uint64_t)u.p8[14]) << 48;
			case 14:
				d += ((uint64_t)u.p8[13]) << 40;
			case 13:
				d += ((uint64_t)u.p8[12]) << 32;
			case 12:
				d += u.p32[2];
				c += u.p64[0];
				break;
			case 11:
				d += ((uint64_t)u.p8[10]) << 16;
			case 10:
				d += ((uint64_t)u.p8[9]) << 8;
			case 9:
				d += (uint64_t)u.p8[8];
			case 8:
				c += u.p64[0];
				break;
			case 7:
				c += ((uint64_t)u.p8[6]) << 48;
			case 6:
				c += ((uint64_t)u.p8[5]) << 40;
			case 5:
				c += ((uint64_t)u.p8[4]) << 32;
			case 4:
				c += u.p32[0];
				break;
			case 3:
				c += ((uint64_t)u.p8[2]) << 16;
			case 2:
				c += ((uint64_t)u.p8[1]) << 8;
			case 1:
				c += (uint64_t)u.p8[0];
			case 0:
				break;
			}
			len = 0;
		} else {
			c += u.p64[0];
			d += u.p64[1];
			u.p8 += 16;
			len -= 16;
		}

		EMIX(d,c,15)
		EMIX(a,d,52)
		EMIX(b,a,26)

		EMIX(c,b,51)
		EMIX(d,c,28)
		EMIX(a,d, 9)
		EMIX(b,a,47)

		EMIX(c,b,54)
		EMIX(d,c,32)
		EMIX(a,d,25)
		EMIX(b,a,63)
	}

	((uint64_t*)out)[0] = a;
	((uint64_t*)out)[1] = b;
}

#else

/* pipelined version as demonstration for hardware */

#define ST 12

#define EMIX(y,x,w,v,r,s) \
  v[s] = v[s-1]; \
  w[s] = w[s-1]; \
  y[s] = (y[s-1] ^ x[s-1]) + (x[s] = Rot64(x[s-1],r));

void short128_hash(const void *key, int len, int seed, void *out) {
	uint64_t a[ST], b[ST], c[ST], d[ST];
	const unsigned char *ptr = (const unsigned char *)key;

	a[ST-1] = b[ST-1] = seed;
	c[ST-1] = d[ST-1] = INIT_VAL;

	while (len > 0) {
		a[0] = a[ST-1] ^  (uint64_t)len;
		b[0] = b[ST-1] ^ ~(uint64_t)len;
		if (len < 16) {
			uint64_t tmp[2];
			memcpy(tmp, ptr, len);
			memset((char*)tmp+len, 0, 16-len);
			c[0] = c[ST-1] + tmp[0];
			d[0] = d[ST-1] + tmp[1];
			len = 0;
		} else {
			c[0] = c[ST-1] + ((uint64_t*)ptr)[0];
			d[0] = d[ST-1] + ((uint64_t*)ptr)[1];
			ptr += 16;
			len -= 16;
		}

		EMIX(d,c,b,a,15, 1);
		EMIX(a,d,c,b,52, 2);
		EMIX(b,a,d,c,26, 3);

		EMIX(c,b,a,d,51, 4);
		EMIX(d,c,b,a,28, 5);
		EMIX(a,d,c,b, 9, 6);
		EMIX(b,a,d,c,47, 7);

		EMIX(c,b,a,d,54, 8);
		EMIX(d,c,b,a,32, 9);
		EMIX(a,d,c,b,25,10);
		EMIX(b,a,d,c,63,11);
	}

	((uint64_t*)out)[0] = a[ST-1];
	((uint64_t*)out)[1] = b[ST-1];
}

#endif

void short64_hash(const void *key, int len, int seed, void *out) {
	uint64_t tmp[2];
	short128_hash(key, len, seed, tmp);
	*(uint64_t*)out = tmp[0];
}

void short32_hash(const void *key, int len, int seed, void *out) {
	uint32_t tmp[4];
	short128_hash(key, len, seed, tmp);
	*(uint32_t*)out = tmp[0];
}

/*
#define MIX(z1,y1,z0,y0,x0,r) z1 = z0 ^ (y1 = Rot64(y0,r) + x0);

	int count = 0;
	while (len >= 16) {
		if (count++ & 1) {
			a0 += ((uint64_t*)ptr)[0];
			b0 += ((uint64_t*)ptr)[1];
			ptr += 16;
			len -= 16;
			continue;
		}
		c0 += ((uint64_t*)ptr)[0];
		d0 += ((uint64_t*)ptr)[1];
		ptr += 16;
		len -= 16;
		MIX(a1,c1,a0,c0,d0,50)
		MIX(b1,d1,b0,d0,a1,52)
		MIX(c2,a2,c1,a1,b1,30)
		MIX(d2,b2,d1,b1,c2,41)
		MIX(a3,c3,a2,c2,d2,54)
		MIX(b3,d3,b2,d2,a3,48)
		MIX(c4,a4,c3,a3,b3,38)
		MIX(d4,b4,d3,b3,c4,37)
		MIX(a5,c5,a4,c4,d4,62)
		MIX(b5,d5,b4,d4,a5,34)
		MIX(c6,a6,c5,a5,b5, 5)
		MIX(d6,b6,d5,b5,c6,36)
	}
*/

#if defined(TEST)
#include <stdio.h> /* printf */
#include <stdlib.h> /* exit */
#include <string.h> /* strlen */

int main(int argc, char *argv[])
{
	int i, k;
	int seed = 0xD;
	unsigned char hash[16];
	char *keys[] = {
		(char *)"5B_01",
		(char *)"8B_01234",
		(char *)"9B_012345",
		(char *)"16B_0123456789AB"
	};

	printf("keys (char string), length (int):\n");
	for (k = 0; k < sizeof(keys)/sizeof(char*); k++){
		printf(" \"%s\", %d\n", keys[k], strlen(keys[k]));
	}

	printf("keys (128-bit hex word), length (int):\n");
	for (k = 0; k < sizeof(keys)/sizeof(char*); k++) {
		int key_sz = strlen(keys[k]);
		putchar(' ');
		for (i = sizeof(hash)-1; i >= 0; i--) {
			if (i < key_sz) printf("%02X", keys[k][i]);
			else printf("00");
		}
		printf(", %d\n", strlen(keys[k]));
	}

	printf("hashs (16 hex bytes low to high):\n");
	for (k = 0; k < sizeof(keys)/sizeof(char*); k++) {
		short128_hash(keys[k], strlen(keys[k]), seed, hash);
		for (i = 0; i < sizeof(hash); i++) printf(" %02X", hash[i]);
		putchar('\n');
	}

	printf("hashs (128-bit hex word):\n");
	for (k = 0; k < sizeof(keys)/sizeof(char*); k++) {
		short128_hash(keys[k], strlen(keys[k]), seed, hash);
		putchar(' ');
		for (i = sizeof(hash)-1; i >= 0; i--) printf("%02X", hash[i]);
		putchar('\n');
	}

	return(EXIT_SUCCESS);
}

/*
keys (char string), length (int):
 "5B_01", 5
 "8B_01234", 8
 "9B_012345", 9
 "16B_0123456789AB", 16
keys (128-bit hex word), length (int):
 000000000000000000000031305F4235, 5
 000000000000000034333231305F4238, 8
 000000000000003534333231305F4239, 9
 4241393837363534333231305F423631, 16
hashs (16 hex bytes low to high):
 14 85 84 56 BB 2A D3 5D 9D EF 37 F4 D2 DE A2 A3
 7A BD 2A 5B 29 0F 93 55 E9 2A 24 C5 60 56 A7 21
 4D BE D3 5A 3A 97 BA 88 99 12 30 55 EE 10 75 FB
 37 8E E2 D9 10 4D 6E 59 D1 14 A5 6F 1E 01 C1 C2
hashs (128-bit hex word):
 A3A2DED2F437EF9D5DD32ABB56848514
 21A75660C5242AE955930F295B2ABD7A
 FB7510EE5530129988BA973A5AD3BE4D
 C2C1011E6FA514D1596E4D10D9E28E37
*/
#endif
