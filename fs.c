/*
Implementation of SimpleFS.
Make your changes here.
*/

#include "fs.h"
#include "disk.h"

#include <time.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

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

bool *freeBlocks;
bool mounted = false;

int fs_format()
{
	// Check if a filesystem is already mounted
	if (mounted) {
		return 0;
	}

	int i;
	union fs_block superBlock;
	disk_read(thedisk,0,superBlock.data);

	// Set aside 10% of blocks for inode blocks
	if (disk_nblocks(thedisk) % 10 == 0) {
		superBlock.super.ninodeblocks = (disk_nblocks(thedisk) / 10);
	} else {
		superBlock.super.ninodeblocks = (disk_nblocks(thedisk) / 10) + 1;
	}

	// Clear the inode table (initializes data to random values)
	union fs_block randomBlock;
	for (i = 0; i < BLOCK_SIZE; i++) {
		randomBlock.data[i] = 0;
	}
	for (i = 1; i < disk_nblocks(thedisk); i++) {
		disk_write(thedisk, i, randomBlock.data);
	}

	// Write the superblock
	superBlock.super.magic = FS_MAGIC;
	superBlock.super.nblocks = disk_nblocks(thedisk);
	superBlock.super.ninodes = INODES_PER_BLOCK * superBlock.super.ninodeblocks;
	disk_write(thedisk,0,superBlock.data);

	return 1;
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
	for (i = 1; i <= block.super.ninodeblocks; i++) {
		union fs_block inodeBlock;
		disk_read(thedisk,i,inodeBlock.data);
		// Loop through each inode in the inode block
		for (j = 0; j < INODES_PER_BLOCK; j++) {
			struct fs_inode myInode = inodeBlock.inode[j];
			if (!myInode.isvalid) continue;
			printf("inode %d:\n", (j + 1 + ((i-1) * INODES_PER_BLOCK)));
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
	int i, j, k;
	if (mounted) {
		return 0;
	}

	mounted = true;

	// Read and verify superblock
	union fs_block superBlock;
	disk_read(thedisk,0,superBlock.data);
	if ((superBlock.super.magic != FS_MAGIC) || superBlock.super.nblocks != disk_nblocks(thedisk) || superBlock.super.ninodes != (INODES_PER_BLOCK * superBlock.super.ninodeblocks)) {
		return 0;
	}
	if (disk_nblocks(thedisk) % 10 == 0) {
		if (superBlock.super.ninodeblocks != (disk_nblocks(thedisk) / 10)) return 0;
	} else {
		if (superBlock.super.ninodeblocks != (disk_nblocks(thedisk) / 10) + 1) return 0;
	}
	
	// Build a free block bitmap
	freeBlocks = calloc(superBlock.super.nblocks, sizeof(bool));

	freeBlocks[0] = false;
	for (i = 1; i < superBlock.super.nblocks; i++) {
		freeBlocks[i] = true;
	}

	for (i = 1; i <= superBlock.super.ninodeblocks; i++) {
		union fs_block inodeBlock;
		disk_read(thedisk, i, inodeBlock.data);

		// Set inode block to not free
		freeBlocks[i] = false;
		
		for (j = 0; j < INODES_PER_BLOCK; j++) {
			struct fs_inode myInode = inodeBlock.inode[j];
			if (!(myInode.isvalid)) continue;

			// Make all the direct pointers not free
			for (k = 0; k < POINTERS_PER_INODE; k++) {
				if (!(myInode.direct[k])) continue;
				freeBlocks[myInode.direct[k]] = false;
			}

			// Make indirect pointer block and blocks pointed to not free
			if (myInode.indirect) {
				freeBlocks[myInode.indirect] = false;
				union fs_block indirectBlock;
				disk_read(thedisk, myInode.indirect, indirectBlock.data);

				for (k = 0; k < POINTERS_PER_BLOCK; k++) {
					if (!(indirectBlock.pointers[k])) continue;
					freeBlocks[indirectBlock.pointers[k]] = false;
				}
			}
		}

	}

	return 1;
}

int fs_create()
{
	int i, j, k;
	union fs_block superBlock;
	disk_read(thedisk,0,superBlock.data);

	// Loop through the inode blocks
	for (i = 1; i <= superBlock.super.ninodeblocks; i++) {
		union fs_block inodeBlock;
		disk_read(thedisk,i,inodeBlock.data);

		// Loop through each inode
		for (j = 0; j < INODES_PER_BLOCK; j++) {
			if (inodeBlock.inode[j].isvalid) {
				continue;
			}
			inodeBlock.inode[j].isvalid = 1;

			// Loop through each direct pointer and set to 0
			for (k = 0; k < POINTERS_PER_INODE; k++) {
				inodeBlock.inode[j].direct[k] = 0;
			}
			inodeBlock.inode[j].indirect = 0;
			inodeBlock.inode[j].size = 0;
			inodeBlock.inode[j].ctime = time(NULL);

			disk_write(thedisk, i, inodeBlock.data);

			return (j + 1 + ((i-1) * INODES_PER_BLOCK));
		}
	}
	return 0;
}

int fs_delete( int inumber )
{
	int i, j;
	union fs_block inodeBlock;
	int32_t blockNum = (inumber / INODES_PER_BLOCK) + 1;
	int32_t blockOffset = (inumber % INODES_PER_BLOCK) - 1;

	disk_read(thedisk, blockNum, inodeBlock.data);

	// Check to make sure the inode is valid
	if (inodeBlock.inode[blockOffset].isvalid == 0) return 0;

	// Free the direct blocks
	for (i = 0; i < POINTERS_PER_INODE; i++) {
		if (inodeBlock.inode[blockOffset].direct[i]) {
			freeBlocks[inodeBlock.inode[blockOffset].direct[i]] = true;
			inodeBlock.inode[blockOffset].direct[i] = 0;
		}
	}

	// Free the indirect block and subsequent pointers
	if (inodeBlock.inode[blockOffset].indirect) {
		union fs_block indirectBlock;
		disk_read(thedisk, inodeBlock.inode[blockOffset].indirect, indirectBlock.data);

		for (j = 0; j < POINTERS_PER_BLOCK; j++) {
			if (indirectBlock.pointers[j]) {
				freeBlocks[indirectBlock.pointers[j]] = true;
				indirectBlock.pointers[j] = 0;
			}
		}

		freeBlocks[inodeBlock.inode[blockOffset].indirect] = true;
	}

	inodeBlock.inode[blockOffset].isvalid = 0;
	inodeBlock.inode[blockOffset].indirect = 0;
	inodeBlock.inode[blockOffset].size = 0;
	inodeBlock.inode[blockOffset].ctime = time(NULL);

	disk_write(thedisk, blockNum, inodeBlock.data);

	return 1;
}

int fs_getsize( int inumber )
{
	union fs_block inodeBlock;
	int32_t blockNum = (inumber / INODES_PER_BLOCK) + 1;
	int32_t blockOffset = (inumber % INODES_PER_BLOCK) - 1;

	disk_read(thedisk, blockNum, inodeBlock.data);

	if (inodeBlock.inode[blockOffset].isvalid) {
		return inodeBlock.inode[blockOffset].size;
	} else {
		return -1;
	}
}

int fs_read( int inumber, char *data, int length, int offset )
{
	int i;

	union fs_block inodeBlock;
	int32_t dataBlock = offset / BLOCK_SIZE;
	int32_t dataOffset = offset % BLOCK_SIZE;
	int32_t inoBlock = (inumber / INODES_PER_BLOCK) + 1;
	int32_t inoOffset = (inumber % INODES_PER_BLOCK) - 1;

	disk_read(thedisk, inoBlock, inodeBlock.data);
	if (!(inodeBlock.inode[inoOffset].isvalid)) return 0;
	if (offset > inodeBlock.inode[inoOffset].size) return 0; 

	union fs_block currDataBlock; // Holds whatever data is currently being read
	int bytes = 0;

	while (offset < inodeBlock.inode[inoOffset].size && bytes < length) {
		int currBlock = 0;

		// Direct data blocks
		if (dataBlock < POINTERS_PER_INODE) {
			currBlock = inodeBlock.inode[inoOffset].direct[dataBlock];
		} // Indirect data blocks
		else if (inodeBlock.inode[inoOffset].indirect) {
			union fs_block indirectBlock;
			disk_read(thedisk, inodeBlock.inode[inoOffset].indirect, indirectBlock.data);
			currBlock = indirectBlock.pointers[dataBlock - POINTERS_PER_INODE];
		}

		disk_read(thedisk, currBlock, currDataBlock.data);

		// Read the bytes
		for (i = dataOffset; i < BLOCK_SIZE; i++) {
			if (offset >= inodeBlock.inode[inoOffset].size || bytes >= length) {
				break;
			}
			data[bytes] = currDataBlock.data[i];
			offset++;
			bytes++;
		}
		dataOffset = 0;
		dataBlock++;
	}

	return bytes;
}

int fs_write( int inumber, const char *data, int length, int offset )
{
	union fs_block inodeBlock;
	int32_t inoBlock = (inumber / INODES_PER_BLOCK) + 1;
	int32_t inoOffset = (inumber % INODES_PER_BLOCK) - 1;

	disk_read(thedisk, inoBlock, inodeBlock.data);
	if (!(inodeBlock.inode[inoOffset].isvalid)) return 0;
	if (offset > inodeBlock.inode[inoOffset].size) return 0; 

	// int bytes = 0;



	return 0;
}

// CHANGE THE INODES PER BLOCK TO 128 *************