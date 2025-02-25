/*
 * Copyright (C) 1999 by Andries Brouwer
 * Copyright (C) 1999, 2000, 2003 by Theodore Ts'o
 * Copyright (C) 2001 by Andreas Dilger
 * Copyright (C) 2004 Kay Sievers <kay.sievers@vrfy.org>
 * Copyright (C) 2008 Karel Zak <kzak@redhat.com>
 *
 * This file may be redistributed under the terms of the
 * GNU Lesser General Public License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include "superblocks.h"
#include "crc32.h"

struct cramfs_super
{
	uint8_t		magic[4];
	uint32_t	size;
	uint32_t	flags;
	uint32_t	future;
	uint8_t		signature[16];
	struct cramfs_info
	{
		uint32_t	crc;
		uint32_t	edition;
		uint32_t	blocks;
		uint32_t	files;
	} __attribute__((packed)) info;
	uint8_t		name[16];
} __attribute__((packed));

#define CRAMFS_FLAG_FSID_VERSION_2	0x00000001	/* fsid version #2 */

static int cramfs_verify_csum(blkid_probe pr, const struct blkid_idmag *mag, struct cramfs_super *cs)
{
	uint32_t crc, expected, csummed_size;
	unsigned char *csummed;

	if (!(cs->flags & CRAMFS_FLAG_FSID_VERSION_2))
		return 1;

	expected = le32_to_cpu(cs->info.crc);
	csummed_size = le32_to_cpu(cs->size);

	if (csummed_size > (1 << 16)
	    || csummed_size < sizeof(struct cramfs_super))
		return 0;

	csummed = blkid_probe_get_sb_buffer(pr, mag, csummed_size);
	if (!csummed)
		return 0;
	memset(csummed + offsetof(struct cramfs_super, info.crc), 0, sizeof(uint32_t));

	crc = ~ul_crc32(~0LL, csummed, csummed_size);

	return blkid_probe_verify_csum(pr, crc, expected);
}

static int probe_cramfs(blkid_probe pr, const struct blkid_idmag *mag)
{
	struct cramfs_super *cs;

	cs = blkid_probe_get_sb(pr, mag, struct cramfs_super);
	if (!cs)
		return errno ? -errno : 1;

	if (!cramfs_verify_csum(pr, mag, cs))
		return 1;

	blkid_probe_set_label(pr, cs->name, sizeof(cs->name));
	return 0;
}

const struct blkid_idinfo cramfs_idinfo =
{
	.name		= "cramfs",
	.usage		= BLKID_USAGE_FILESYSTEM,
	.probefunc	= probe_cramfs,
	.magics		=
	{
		{ .magic = "\x45\x3d\xcd\x28", .len = 4 },
		{ .magic = "\x28\xcd\x3d\x45", .len = 4 },
		{ NULL }
	}
};


