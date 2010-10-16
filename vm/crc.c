/*
The following C code (by Rob Warnock <rpw3@sgi.com>) does CRC-32 in
BigEndian/BigEndian byte/bit order.  That is, the data is sent most
significant byte first, and each of the bits within a byte is sent most
significant bit first, as in FDDI. You will need to twiddle with it to do
Ethernet CRC, i.e., BigEndian/LittleEndian byte/bit order. [Left as an
exercise for the reader.]

The CRCs this code generates agree with the vendor-supplied Verilog models
of several of the popular FDDI "MAC" chips.
*/

#include "crc.h"

void celinit_crc32(void);


unsigned long celcrc32_table[256];
/* Initialized first time "crc32()" is called. If you prefer, you can
 * statically initialize it at compile time. [Another exercise.]
 */

unsigned long celcrc32(unsigned char *buf, int len)
{
        unsigned char *p;
        unsigned long  crc;

        if (!celcrc32_table[1])    /* if not already done, */
                celinit_crc32();   /* build table */
        crc = 0xffffffff;       /* preload shift register, per CRC-32 spec */
        for (p = buf; len > 0; ++p, --len)
                crc = (crc << 8) ^ celcrc32_table[(crc >> 24) ^ *p];
        return crc;             /* transmit complement, per CRC-32 spec */
				/* Yeah, but we didn't do that 
				   There is no need to have the 
				   data complimented in the end. That is
				   for ethernet/etc.  DN 5/9/98 */
}

/*
 * Build auxiliary table for parallel byte-at-a-time CRC-32.
 */
#define CRC32_POLY 0x04c11db7     /* AUTODIN II, Ethernet, & FDDI */

void celinit_crc32(void)
{
        int i, j;
        unsigned long c;

        for (i = 0; i < 256; ++i) {
                for (c = i << 24, j = 8; j > 0; --j)
                        c = c & 0x80000000 ? (c << 1) ^ CRC32_POLY : (c << 1);
                celcrc32_table[i] = c;
        }
}


