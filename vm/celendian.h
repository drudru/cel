/*
 * Copyright (c) 1987, 1991 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	from: @(#)endian.h	7.8 (Berkeley) 4/3/91
 *	$Id: celendian.h,v 1.3 2002/02/22 22:23:24 dru Exp $
 */

/* Well you know where we got this from, now we'll change it
   so it doesn't interfere with the namespace of the system's i
   includes
 */

#ifndef _CEL_ENDIAN_H
#define _CEL_ENDIAN_H 1

/*
 * Define the order of 32-bit words in 64-bit words.
 */
#define	BB_QUAD_HIGHWORD 1
#define	BB_QUAD_LOWWORD 0

/*
 * Definitions for byte order, according to byte significance from low
 * address to high.
 */
#define	BBLITTLE_ENDIAN	1234	/* LSB first: i386, vax */
#define	BBBIG_ENDIAN	4321	/* MSB first: 68000, ibm, net */
#define	BBPDP_ENDIAN	3412	/* LSB first in word, MSW first in long */

#ifdef __BIG_ENDIAN__
#define	BBBYTE_ORDER	BBBIG_ENDIAN
#else
#define	BBBYTE_ORDER	BBLITTLE_ENDIAN
#endif


#define BB__word_swap_long(x) \
({ register uint32 __X = (x); \
   __asm ("rorl $16, %1" \
	: "=r" (__X) \
	: "0" (__X)); \
   __X; })

#define BB__byte_swap_long(x) \
__extension__ ({ register uint32 __X = (x); \
   __asm ("xchgb %h1, %b1\n\trorl $16, %1\n\txchgb %h1, %b1" \
	: "=q" (__X) \
	: "0" (__X)); \
   __X; })

#define BB__byte_swap_word(x) \
__extension__ ({ register uint16 __X = (x); \
   __asm ("xchgb %h1, %b1" \
	: "=q" (__X) \
	: "0" (__X)); \
   __X; })

/*
 * Macros for network/external number representation conversion.
 */
#if BBBYTE_ORDER == BBBIG_ENDIAN && !defined(lint)
#define	std2hostInteger(x)	(x)
#define	std2hostShort(x)	(x)
#define	host2stdInteger(x)	(x)
#define	host2stdShort(x)	(x)

#else

#define	std2hostInteger		BB__byte_swap_long
#define	std2hostShort		BB__byte_swap_word
#define	host2stdInteger		BB__byte_swap_long
#define	host2stdShort		BB__byte_swap_word

#endif


#endif /* _CEL_ENDIAN_H */
