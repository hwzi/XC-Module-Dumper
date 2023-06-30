#include "stdafx.h"

/*
*  Copyright (C) 2015 Cisco Systems, Inc. and/or its affiliates. All rights reserved.
*  Copyright (C) 2007-2008 Sourcefire, Inc.
*
*  Authors: Alberto Wu
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License version 2 as
*  published by the Free Software Foundation.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
*  MA 02110-1301, USA.
*/

/*
** unsp.c
**
** 11/10/2k6 - Merge started.
**
*/

/*
** Plays around with NsPack compressed executables
**
** This piece of code is dedicated to Damian Put
** who I made a successful and wealthy man.
**
** Damian, you owe me a pint!
*/

/*
** TODO:
**
** - Investigate the "unused" code in NsPack
** - Fetch all the nspacked samples from the zoo and run extensive testing
** - Add bound checks
** - Test against the zoo again
** - Perform regression testing against the full zoo
** - check nested
** - look at the 64bit version (one of these days)
**
*/

#if HAVE_CONFIG_H
#include "clamav-config.h"
#endif

#include <stdlib.h>

#include "unsp.h"

#define CLI_ISCONTAINED(bb, bb_size, sb, sb_size)	\
  ((bb_size) > 0 && (sb_size) > 0 && (size_t)(sb_size) <= (size_t)(bb_size) \
   && (sb) >= (bb) && ((sb) + (sb_size)) <= ((bb) + (bb_size)) && ((sb) + (sb_size)) > (bb) && (sb) < ((bb) + (bb_size)))

/* Implementation independent sign-extended signed right shift */
#ifdef HAVE_SAR
#define CLI_SRS(n,s) ((n)>>(s))
#else
#define CLI_SRS(n,s) ((((n)>>(s)) ^ (1<<(sizeof(n)*8-1-s))) - (1<<(sizeof(n)*8-1-s)))
#endif
#define CLI_SAR(n,s) n = CLI_SRS(n,s)

unsigned int very_real_unpack(unsigned short *table, unsigned int tablesz, unsigned int tre, unsigned int allocsz,
	unsigned int firstbyte, const char *src, unsigned int ssize, char *dst, unsigned int dsize)
{
	struct UNSP read_struct;
	unsigned int i = (0x300 << ((allocsz + tre) & 0xff)) + 0x736;

	unsigned int previous_bit = 0;
	unsigned int unpacked_so_far = 0;
	unsigned int backbytes = 1;
	unsigned int oldbackbytes = 1;
	unsigned int old_oldbackbytes = 1;
	unsigned int old_old_oldbackbytes = 1;

	unsigned int damian = 0;
	unsigned int put = (1 << (allocsz & 0xff)) - 1;

	unsigned int bielle = 0;

	firstbyte = (1 << (firstbyte & 0xff)) - 1;

	if (tablesz < i * sizeof(unsigned short))
		return 2;

	/* init table */
	while (i)
		table[--i] = 0x400;

	/* table noinit */

	/* get_five - inlined */
	read_struct.error = 0;
	read_struct.oldval = 0;
	read_struct.src_curr = src;
	read_struct.bitmap = 0xffffffff;
	read_struct.src_end = src + ssize; // - 13?
	read_struct.table = (char *)table;
	read_struct.tablesz = tablesz;

	for (i = 0; i < 5; i++)
		read_struct.oldval = (read_struct.oldval << 8) | get_byte(&read_struct);

	if (read_struct.error)
		return 1;
	/* if (!dsize) return 0; - checked in pe.c */


	/* very_unpacking_loop */
	while (1)
	{
		unsigned int backsize = firstbyte & unpacked_so_far;
		unsigned int tpos;
		unsigned int temp = damian;

		if (read_struct.error)
			return 1; /* checked once per mainloop, keeps the code readable and it's still safe */

		if (!getbit_from_table(&table[(damian << 4) + backsize], &read_struct)) { /* no_mainbit */

			unsigned int shft = 8 - (tre & 0xff);
			shft &= 0xff;
			tpos = (bielle >> shft) + ((put & unpacked_so_far) << (tre & 0xff));
			tpos *= 3;
			tpos <<= 8;

			if ((int)damian >= 4) { /* signed */
				if ((int)damian >= 0xa) { /* signed */
					damian -= 6;
				}
				else {
					damian -= 3;
				}
			}
			else {
				damian = 0;
			}

			/* 44847E */
			if (previous_bit)
			{
				if (!CLI_ISCONTAINED(dst, dsize, &dst[unpacked_so_far - backbytes], 1))
					return 1;
				ssize = (ssize & 0xffffff00) | (unsigned char)dst[unpacked_so_far - backbytes]; /* FIXME! ssize is not static */
				bielle = get_100_bits_from_tablesize(&table[tpos + 0x736], &read_struct, ssize);
				previous_bit = 0;
			}
			else {
				bielle = get_100_bits_from_table(&table[tpos + 0x736], &read_struct);
			}

			/* unpack_one_byte - duplicated */
			if (!CLI_ISCONTAINED(dst, dsize, &dst[unpacked_so_far], 1))
				return 1;

			dst[unpacked_so_far] = bielle;
			unpacked_so_far++;
			if (unpacked_so_far >= dsize)
				return 0;

			continue;

		}
		else { /* got_mainbit */

			bielle = previous_bit = 1;

			if (getbit_from_table(&table[damian + 0xc0], &read_struct)) {
				if (!getbit_from_table(&table[damian + 0xcc], &read_struct)) {
					tpos = damian + 0xf;
					tpos <<= 4;
					tpos += backsize;
					if (!getbit_from_table(&table[tpos], &read_struct))
					{
						if (!unpacked_so_far)
							return bielle; /* FIXME: WTF?! */

						damian = 2 * ((int)damian >= 7) + 9; /* signed */
						if (!CLI_ISCONTAINED(dst, dsize, &dst[unpacked_so_far - backbytes], 1))
							return 1;

						bielle = (unsigned char)dst[unpacked_so_far - backbytes];
						/* unpack_one_byte - real */
						dst[unpacked_so_far] = bielle;
						unpacked_so_far++;
						if (unpacked_so_far >= dsize)
							return 0;
						continue;

					}
					else { /* gotbit_tre */
						backsize = get_n_bits_from_tablesize(&table[0x534], &read_struct, backsize);
						damian = ((int)damian >= 7); /* signed */
						damian = ((damian - 1) & 0xfffffffd) + 0xb;
						/* jmp checkloop_and_backcopy (uses edx) */
					} /* gotbit_uno ends */
				}
				else { /* gotbit_due */
					if (!getbit_from_table(&table[damian + 0xd8], &read_struct)) {
						tpos = oldbackbytes;
					}
					else {
						if (!getbit_from_table(&table[damian + 0xe4], &read_struct)) {
							tpos = old_oldbackbytes;
						}
						else {
							/* set_old_old_oldback */
							tpos = old_old_oldbackbytes;
							old_old_oldbackbytes = old_oldbackbytes;
						}
						/* set_old_oldback */
						old_oldbackbytes = oldbackbytes;
					}
					/* set_oldback */
					oldbackbytes = backbytes;
					backbytes = tpos;

					backsize = get_n_bits_from_tablesize(&table[0x534], &read_struct, backsize);
					damian = ((int)damian >= 7); /* signed */
					damian = ((damian - 1) & 0xfffffffd) + 0xb;
					/* jmp checkloop_and_backcopy (uses edx) */
				} /* gotbit_due ends */
			}
			else { /* gotbit_uno */

				old_old_oldbackbytes = old_oldbackbytes;
				old_oldbackbytes = oldbackbytes;
				oldbackbytes = backbytes;

				damian = ((int)damian >= 7); /* signed */
				damian = ((damian - 1) & 0xfffffffd) + 0xa;

				backsize = get_n_bits_from_tablesize(&table[0x332], &read_struct, backsize);

				tpos = ((unsigned int)backsize >= 4) ? 3 : backsize; /* signed */
				tpos <<= 6;
				tpos = get_n_bits_from_table(&table[0x1b0 + tpos], 6, &read_struct);

				if (tpos >= 4) { /* signed */

					unsigned int s = tpos;
					s >>= 1;
					s--;

					temp = (tpos & bielle) | 2;
					temp <<= (s & 0xff);


					if ((unsigned int)tpos < 0xe) {
						temp += get_bb(&table[(temp - tpos) + 0x2af], s, &read_struct);
					}
					else {
						s += 0xfffffffc;
						tpos = get_bitmap(&read_struct, s);
						tpos <<= 4;
						temp += tpos;
						temp += get_bb(&table[0x322], 4, &read_struct);
					}
				}
				else {
					/* gotbit_uno_out1 */
					backbytes = temp = tpos;
				}
				/* gotbit_uno_out2 */
				backbytes = temp + 1;
				/* jmp checkloop_and_backcopy (uses edx) */
			} /* gotbit_uno ends */

			/* checkloop_and_backcopy */
			if (!backbytes)
				return 0; /* very_real_unpack_end */

			if (backbytes > unpacked_so_far)
				return bielle; /* FIXME: WTF?! */

			backsize += 2;

			if (!CLI_ISCONTAINED(dst, dsize, &dst[unpacked_so_far], backsize) ||
				!CLI_ISCONTAINED(dst, dsize, &dst[unpacked_so_far - backbytes], backsize)
				) {
				// cli_dbgmsg("%p %x %p %x\n", dst, dsize, &dst[unpacked_so_far], backsize);
				return 1;
			}

			do {
				dst[unpacked_so_far] = dst[unpacked_so_far - backbytes];
				unpacked_so_far++;
			} while (--backsize && unpacked_so_far < dsize);
			bielle = (unsigned char)dst[unpacked_so_far - 1];

			if (unpacked_so_far >= dsize)
				return 0;

		} /* got_mainbit ends */

	} /* while true ends */
}

unsigned int get_byte(struct UNSP *read_struct)
{

	unsigned int ret;

	if (read_struct->src_curr >= read_struct->src_end)
	{
		read_struct->error = 1;
		return 0xff;
	}

	ret = *(read_struct->src_curr);
	read_struct->src_curr++;
	return ret & 0xff;
}

int getbit_from_table(unsigned short *intable, struct UNSP *read_struct)
{
	unsigned int nval;
	if (!CLI_ISCONTAINED((char *)read_struct->table, read_struct->tablesz, (char *)intable, sizeof(unsigned short)))
	{
		read_struct->error = 1;
		return 0xff;
	}
	nval = *intable * (read_struct->bitmap >> 0xb);

	if (read_struct->oldval < nval) { /* unsigned */
		unsigned int sval;
		read_struct->bitmap = nval;
		nval = *intable;
		sval = 0x800 - nval;
		sval = CLI_SRS((int)sval, 5); /* signed */
		sval += nval;
		*intable = sval;
		if (read_struct->bitmap < 0x01000000) { /* unsigned */
			read_struct->oldval = (read_struct->oldval << 8) | get_byte(read_struct);
			read_struct->bitmap <<= 8;
		}
		return 0;
	}

	read_struct->bitmap -= nval;
	read_struct->oldval -= nval;

	nval = *intable;
	nval -= (nval >> 5); /* word, unsigned */
	*intable = nval;

	if (read_struct->bitmap < 0x1000000) { /* unsigned */
		read_struct->oldval = (read_struct->oldval << 8) | get_byte(read_struct);
		read_struct->bitmap <<= 8;
	}

	return 1;
}


unsigned int get_100_bits_from_tablesize(unsigned short *intable, struct UNSP *read_struct, unsigned int ssize) {

	unsigned int count = 1;

	while (count < 0x100)
	{
		unsigned int lpos, tpos;
		lpos = ssize & 0xff;
		ssize = (ssize & 0xffffff00) | ((lpos << 1) & 0xff);
		lpos >>= 7;
		tpos = lpos + 1;
		tpos <<= 8;
		tpos += count;
		tpos = getbit_from_table(&intable[tpos], read_struct);
		count = (count * 2) | tpos;
		if (lpos != tpos) {
			/* second loop */
			while (count < 0x100)
				count = (count * 2) | getbit_from_table(&intable[count], read_struct);
		}
	}
	return count & 0xff;
}


unsigned int get_100_bits_from_table(unsigned short *intable, struct UNSP *read_struct)
{
	unsigned int count = 1;

	while (count < 0x100)
		count = (count * 2) | getbit_from_table(&intable[count], read_struct);

	return count & 0xff;
}


unsigned int get_n_bits_from_table(unsigned short *intable, unsigned int bits, struct UNSP *read_struct)
{
	unsigned int count = 1;
	unsigned int bitcounter;

	/*  if (bits) { always set! */
	bitcounter = bits;
	while (bitcounter--)
		count = count * 2 + getbit_from_table(&intable[count], read_struct);
	/*  } */

	return count - (1 << (bits & 0xff));
}


unsigned int get_n_bits_from_tablesize(unsigned short *intable, struct UNSP *read_struct, unsigned int backsize) {

	if (!getbit_from_table(intable, read_struct))
		return get_n_bits_from_table(&intable[(backsize << 3) + 2], 3, read_struct);

	if (!getbit_from_table(&intable[1], read_struct))
		return 8 + get_n_bits_from_table(&intable[(backsize << 3) + 0x82], 3, read_struct);

	return 0x10 + get_n_bits_from_table(&intable[0x102], 8, read_struct);
}

unsigned int get_bb(unsigned short *intable, unsigned int back, struct UNSP *read_struct) {
	unsigned int pos = 1;
	unsigned int bb = 0;
	unsigned int i;

	if ((int)back <= 0) /* signed */
		return 0;

	for (i = 0; i < back; i++) {
		unsigned int bit = getbit_from_table(&intable[pos], read_struct);
		pos = (pos * 2) + bit;
		bb |= (bit << i);
	}
	return bb;
}

unsigned int get_bitmap(struct UNSP *read_struct, unsigned int bits)
{
	unsigned int retv = 0;

	if ((int)bits <= 0)
		return 0; /* signed */

	while (bits--)
	{
		read_struct->bitmap >>= 1; /* unsigned */
		retv <<= 1;
		if (read_struct->oldval >= read_struct->bitmap) { /* unsigned */
			read_struct->oldval -= read_struct->bitmap;
			retv |= 1;
		}
		if (read_struct->bitmap < 0x1000000)
		{
			read_struct->bitmap <<= 8;
			read_struct->oldval = (read_struct->oldval << 8) | get_byte(read_struct);
		}
	}
	return retv;
}
