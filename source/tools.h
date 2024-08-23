/*******************************************************************************
 * tools.h
 *
 * Copyright (c) 2009 The Lemon Man
 * Copyright (c) 2009 Nicksasa
 * Copyright (c) 2009 WiiPower
 *
 * Distributed under the terms of the GNU General Public License (v2)
 * See http://www.gnu.org/licenses/gpl-2.0.txt for more info.
 *
 ******************************************************************************/
typedef struct _dirent{
	char name[ISFS_MAXPATH + 1];
	int type;
} dirent_t;


#define TITLE_UPPER(x)		((u32)((x) >> 32))
#define TITLE_LOWER(x)		((u32)(x))
#define TITLE_ID(x,y)		(((u64)(x) << 32) | (y))

typedef struct{
	u8 channel_type;		// 00: no channel , 01: disc channel , 03: channel
	u8 secondary_type;		// 00: normal channels , 01: disc, mii, photo and shop channels
	u8 unknown[4];			// Unknown. All my titles have these set to 00.
	u16 flags;				// Probably flags. All titles except disk use 000e, and disc uses 000f.
	u64 title_id;			// Title ID.
} iplsave_entry;
 
typedef struct{
	char magic[4];					// "RIPL" header
	u32 filesize;					// The size of iplsave.bin. Always 0x00000340 or 0x000004c0 (4.x)
	u8 unknown[8];					// Version? It's always 0x0000000200000000 or 0x0000000300000000 (4.x)
	iplsave_entry channels[0x30];	// Channels
	u8 unknown2[0x10];				// Some may be padding.
	u8 sdchannels[0x10*25];			// SD channels?
	u8 md5_sum[0x10];				// MD5 sum of the rest of the file
} iplsave;


void *allocate_memory(u32 size);
s32 getdir(char *, dirent_t **, u32 *);
int read_file(char *filepath, u8 **buffer);
