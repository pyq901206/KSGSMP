/*
 * FILE:   base64.c
 * AUTHOR: Colin Perkins
 *
 * MIME base64 encoder/decoder described in rfc1521. This code is derived
 * from version 2.7 of the Bellcore metamail package.
 *
 * Copyright (c) 1998-2000 University College London
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, is permitted provided that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the Computer Science
 *      Department at University College London
 * 4. Neither the name of the University nor of the Department may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE AUTHORS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESSED OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * Copyright (c) 1991 Bell Communications Research, Inc. (Bellcore)
 * 
 * Permission to use, copy, modify, and distribute this material 
 * for any purpose and without fee is hereby granted, provided 
 * that the above copyright notice and this permission notice 
 * appear in all copies, and that the name of Bellcore not be 
 * used in advertising or publicity pertaining to this 
 * material without the specific, prior written permission 
 * of an authorized representative of Bellcore.  BELLCORE 
 * MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR SUITABILITY 
 * OF THIS MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", 
 * WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.
 * 
 * chenxu modify for project 2010-11-23
 */
#include "base64.h"
#include "os_types_interface.h"

static unsigned char basis_64[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

VOS_VOID SMTPC_CODE_Base64Encode(VOS_CHAR *out, VOS_CONST_CHAR* in, VOS_INT32 inlen)
{
	int	i = 0, j = 0;
	int	pad;

	//assert(out_length >= (inlen * 4 / 3));

	while (i < inlen) {
		pad = 3 - (inlen - i);
		if (pad == 2) {
			out[j  ] = basis_64[in[i]>>2];
			out[j+1] = basis_64[(in[i] & 0x03) << 4];
			out[j+2] = '=';
			out[j+3] = '=';
		} else if (pad == 1) {
			out[j  ] = basis_64[in[i]>>2];
			out[j+1] = basis_64[((in[i] & 0x03) << 4) | ((in[i+1] & 0xf0) >> 4)];
			out[j+2] = basis_64[(in[i+1] & 0x0f) << 2];
			out[j+3] = '=';
		} else{
			out[j  ] = basis_64[in[i]>>2];
			out[j+1] = basis_64[((in[i] & 0x03) << 4) | ((in[i+1] & 0xf0) >> 4)];
			out[j+2] = basis_64[((in[i+1] & 0x0f) << 2) | ((in[i+2] & 0xc0) >> 6)];
			out[j+3] = basis_64[in[i+2] & 0x3f];
		}
		i += 3;
		j += 4;
	}
}

/* This assumes that an unsigned char is exactly 8 bits. Not portable code! :-) */
static unsigned char index_64[128] = {
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,   62, 0xff, 0xff, 0xff,   63,
      52,   53,   54,   55,   56,   57,   58,   59,   60,   61, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff,    0,    1,    2,    3,    4,    5,    6,    7,    8,    9,   10,   11,   12,   13,   14,
      15,   16,   17,   18,   19,   20,   21,   22,   23,   24,   25, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff,   26,   27,   28,   29,   30,   31,   32,   33,   34,   35,   36,   37,   38,   39,   40,
      41,   42,   43,   44,   45,   46,   47,   48,   49,   50,   51, 0xff, 0xff, 0xff, 0xff, 0xff
};

#define char64(c)  ((c > 127) ? 0xff : index_64[(c)])

VOS_INT32 SMTPC_CODE_Base64Decode(VOS_CHAR *out, VOS_CONST_CHAR* in, VOS_INT32 inlen)
{
	int		i = 0, j = 0, pad;
	unsigned char	c[4];

	//assert(out_length >= (inlen * 3 / 4));
	//assert((inlen % 4) == 0);
	while ((i + 3) < inlen) {
		pad  = 0;
		c[0] = char64((int)in[i  ]); pad += (c[0] == 0xff);
		c[1] = char64((int)in[i+1]); pad += (c[1] == 0xff);
		c[2] = char64((int)in[i+2]); pad += (c[2] == 0xff);
		c[3] = char64((int)in[i+3]); pad += (c[3] == 0xff);
		if (pad == 2) {
			out[j++] = (c[0] << 2) | ((c[1] & 0x30) >> 4);
			out[j]   = (c[1] & 0x0f) << 4;
		} else if (pad == 1) {
			out[j++] = (c[0] << 2) | ((c[1] & 0x30) >> 4);
			out[j++] = ((c[1] & 0x0f) << 4) | ((c[2] & 0x3c) >> 2);
			out[j]   = (c[2] & 0x03) << 6;
		} else {
			out[j++] = (c[0] << 2) | ((c[1] & 0x30) >> 4);
			out[j++] = ((c[1] & 0x0f) << 4) | ((c[2] & 0x3c) >> 2);
			out[j++] = ((c[2] & 0x03) << 6) | (c[3] & 0x3f);
		}
		i += 4;
	}
	return j;
}
