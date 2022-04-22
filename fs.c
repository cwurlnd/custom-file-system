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

int *freeBlocks;


int fs_format()
{
	int i;
	union fs_block superBlock;
	disk_read(thedisk,0,superBlock.data);

	// Check if a filesystem is already mounted
	if (superBlock.super.magic == FS_MAGIC) {
		return 0;
	}

	// Set aside 10% of blocks for inode blocks
	if (thedisk->nblocks % 10 == 0) {
		superBlock.super.ninodeblocks = (thedisk->nblocks / 10);
	} else {
		superBlock.super.ninodeblocks = (thedisk->nblocks / 10) + 1;
	}

	// Clear the inode table (initializes data to random values)
	union fs_block randomBlock;
	for (i = 0; i < BLOCK_SIZE; i++) {
		randomBlock.data[i] = 0;
	}
	for (i = 1; i < thedisk->nblocks; i++) {
		disk_write(thedisk, i, randomBlock.data);
	}

	// Write the superblock
	superBlock.super.magic = FS_MAGIC;
	superBlock.super.nblocks = thedisk->nblocks;
	superBlock.super.ninodes = INODES_PER_BLOCK * superBlock.super.ninodeblocks;
	disk_write(thedisk,0,superBlock.data);

	return 0;
}

void fs_debug()
{
	int i, j, k;
	union fs_block block;

	disk_read(thedisk,0,block.data);

	// Print the superblock data
	printf("superblock:\n");
	printf("    %d blocks\n",block.super.nblocks);
	printf("    %d inode blocks\n",block.super.ninodeblocks);
	printf("    %d inodes\n",block.super.ninodes);

	// Loop through the inode blocks
	for (i  = 1; i < block.super.ninodeblocks; i++) {
		union fs_block inodeBlock;
		disk_read(thedisk,i,inodeBlock.data);
		// Loop through each inode in the inode block
		for (j = 0; j < INODES_PER_BLOCK; j++) {
			struct fs_inode myInode = inodeBlock.inode[j];
			if (!myInode.isvalid) continue;
			printf("inode %d:\n", j + ((i-1) * INODES_PER_BLOCK));
			printf("    size: %d bytes\n",myInode.size);
			printf("    created: %s",ctime(&myInode.ctime));
			// Loop through the direct pointer in each inode 
			if (myInode.direct) {
				printf("    direct blocks: ");
				for (k = 0; k < POINTERS_PER_INODE; k++) {
					if (!myInode.direct[k]) continue;
					printf("%u ", myInode.direct[k]);
				}
				printf("\n");
			}
			// Loop through the indirect pointers in each inode
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
