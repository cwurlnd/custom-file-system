/*
Implementation of SimpleFS.
Make your changes here.
*/

#include "fs.h"
#include "disk.h"
#include <time.h>

#include <stdio.h>
#include <stdint.h>

extern struct disk *thedisk;

#define FS_MAGIC           0x30341003
#define INODES_PER_BLOCK   128
#define POINTERS_PER_INODE 3
#define POINTERS_PER_BLOCK 1024

struct fs_superblock {
	int32_t magic;
	int32_t nblocks;
	int32_t ninodeblocks;
	int32_t ninodes;
};

struct fs_inode {
	int32_t isvalid;
	int32_t size;
	int64_t ctime;
	int32_t direct[POINTERS_PER_INODE];
	int32_t indirect;
};

union fs_block {
	struct fs_superblock super;
	struct fs_inode inode[INODES_PER_BLOCK];
	int pointers[POINTERS_PER_BLOCK];
	unsigned char data[BLOCK_SIZE];
};

int fs_format()
{
	return 0;
}

void fs_debug()
{
	int i, j, k;
	union fs_block block;

	disk_read(thedisk,0,block.data);

	printf("superblock:\n");
	printf("    %d blocks\n",block.super.nblocks);
	printf("    %d inode blocks\n",block.super.ninodeblocks);
	printf("    %d inodes\n",block.super.ninodes);

	for (i  = 1; i < block.super.ninodeblocks; i++) {
		union fs_block inodeBlock;
		disk_read(thedisk,i,inodeBlock.data);
		for (j = 0; j < INODES_PER_BLOCK; j++) {
			struct fs_inode myInode = inodeBlock.inode[j];
			if (!myInode.isvalid) continue;
			printf("inode %d:\n", j + ((i-1) * INODES_PER_BLOCK));
			printf("    size: %d bytes\n",myInode.size);
			printf("    created: %s",ctime(&myInode.ctime));
			if (myInode.direct) {
				printf("    direct blocks: ");
				for (k = 0; k < POINTERS_PER_INODE; k++) {
					if (!myInode.direct[k]) continue;
					printf("%u ", myInode.direct[k]);
				}
				printf("\n");
			}
			if (myInode.indirect) {
				printf("    indirect block: %u\n", myInode.indirect);
				union fs_block indirectBlock;
				disk_read(thedisk,myInode.indirect,indirectBlock.data);
				printf("    indirect data blocks: ");
				for (k = 0; k < POINTERS_PER_BLOCK; k++) {
					if (!indirectBlock.pointers[k]) continue;
					printf("%u ", indirectBlock.pointers[k]);
				}
				printf("\n");
			}
		}
	}
}

int fs_mount()
{
	return 0;
}

int fs_create()
{
	return 0;
}

int fs_delete( int inumber )
{
	return 0;
}

int fs_getsize( int inumber )
{
	return 0;
}

int fs_read( int inumber, char *data, int length, int offset )
{
	return 0;
}

int fs_write( int inumber, const char *data, int length, int offset )
{
	return 0;
}
