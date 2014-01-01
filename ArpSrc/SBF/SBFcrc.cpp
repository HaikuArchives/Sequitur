#ifndef SBF_SBFCRC_H
#include <SBF/SBFcrc.h>
#endif

/* Table of CRCs of all 8-bit messages. */
static uint32 crc_table[256];

/* Flag: has the table been computed? Initially false. */
static int crc_table_computed = 0;

/* Make the table for a fast CRC. */
static void make_crc_table(void)
{
	uint32 c;
	int n, k;

	for (n = 0; n < 256; n++) {
		c = (uint32) n;
		for (k = 0; k < 8; k++) {
			if (c & 1)
				c = 0xedb88320L ^ (c >> 1);
			else
				c = c >> 1;
		}
		crc_table[n] = c;
	}
	crc_table_computed = 1;
}

uint32 SBFUpdateCRC(uint32 crc, uint8 *buf, size_t len)
{ 
	uint32 c = crc;
	size_t n;

	if (!crc_table_computed)
		make_crc_table();
	for (n = 0; n < len; n++) {
		c = crc_table[(c ^ buf[n]) & 0xff] ^ (c >> 8);
	}
	return c;
}

/* Return the CRC of the bytes buf[0..len-1]. */
uint32 SBFCRC(uint8 *buf, size_t len)
{
	return update_crc(0xffffffffL, buf, len) ^ 0xffffffffL;
}

#endif
