#include "sfs_api.h"


void mksfs(int fresh)
{
	fsSize     = sizeof(rootDirectory);
	fatSize     = sizeof(fat);
	BLOCKSIZE   = ( fsSize > fatSize ? fsSize : fatSize );

	if(fresh)
	{
		// Initialize fresh disk
		int init = init_fresh_disk("root_dir.sfs", BLOCKSIZE, DISKSIZE);

		if(init)
		{
			int i;
			for(i = 1; i < MAXIMUM_FILES; i++) {			// Initialize all files to empty
				root.directory_table[i].isEmpty = 1;
			}

			root.next = 0;									// Cursor set to 0

			for(i = 0; i < DISKSIZE; i++) {					// Initialize all blocks to free
				freeList.freeblocks[i] = 0;
			}

			for(i = 0; i < DISKSIZE; i++) {
				FAT.fatNodes[i].index = EMPTY;
				FAT.fatNodes[i].next = EMPTY;
			}

			FAT.next = 0;									// Cursor set to 0

			// Write blocks for root, fat, and free blocks
			write_blocks(0, 1, (void *)&root);
			write_blocks(1, 1, (void *)&FAT);
			write_blocks(DISKSIZE-1, 1, (void *)&freeList);
		}
	}
	else
	{
		// Initialize disk
		int init = init_disk("root_dir.sfs", BLOCKSIZE, DISKSIZE);

		if(init)
		{
			// Since it's not a fresh file system, read existing blocks
		    read_blocks(0, 1, (void *)&root);
		    read_blocks(1, 1, (void *)&FAT);
		    read_blocks(DISKSIZE-1, 1, (void *)&freeList);
		}
	}
}

void sfs_ls(void)
{
	int i;
	for(i = 0; root.directory_table[i].isEmpty == 0; i++)
	{
		printf("%s  %dKB  %s", root.directory_table[i].name, (root.directory_table[i].size/1000),
				ctime(root.directory_table[i].creation_time));

	}
}

int sfs_fopen(char *name)
{
	return 0;
}

int sfs_fclose(int fileID)
{
	return 0;
}

int sfs_fwrite(int fileID, char *buf, int length)
{
	return 0;
}

int sfs_fread(int fileID, char *buf, int length)
{
	return 0;
}

int sfs_fseek(int fileID, int offset)
{
	return 0;
}

int sfs_remove(char *file)
{
	return 0;
}
