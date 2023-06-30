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

#ifndef __UNSP_H
#define __UNSP_H

struct UNSP
{
	const char *src_curr;
	const char *src_end;
	unsigned int bitmap;
	unsigned int oldval;
	int error;
	/* the following are not in the original structure */
	unsigned int tablesz;
	char *table;
};

unsigned int very_real_unpack(unsigned short *, unsigned int, unsigned int, unsigned int, unsigned int, const char *, unsigned int, char *, unsigned int);
unsigned int get_byte(struct UNSP *);
int getbit_from_table(unsigned short *, struct UNSP *);
unsigned int get_100_bits_from_tablesize(unsigned short *, struct UNSP *, unsigned int);
unsigned int get_100_bits_from_table(unsigned short *, struct UNSP *);
unsigned int get_n_bits_from_table(unsigned short *, unsigned int, struct UNSP *);
unsigned int get_n_bits_from_tablesize(unsigned short *, struct UNSP *, unsigned int);
unsigned int get_bb(unsigned short *, unsigned int, struct UNSP *);
unsigned int get_bitmap(struct UNSP *, unsigned int);

#endif
