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
	// Print name, size (in KB, divide by 1000) and time (convert to c-string to make it readable)
	for(i = 0; root.directory_table[i].isEmpty == 0; i++)
	{
		printf("%s  %dKB  %s", root.directory_table[i].name, (root.directory_table[i].size/1000),
				ctime(&root.directory_table[i].creation_time));

	}
}

int sfs_fopen(char *name)
{
	int fileID;
	int fileIndex = getFileIndex(name);

	strcpy( fdt[opened_files].filename, name );
	fdt[opened_files].opened = 1;
	fdt[opened_files].read_ptr = 0;

	fileID = opened_files;
	++opened_files;

	if(fileIndex == -1)					// New File
	{
		fdt[opened_files].write_ptr = 0;
		FAT.fatNodes[FAT.next].index = getNextFreeBlock();

		FAT.fatNodes[FAT.next].next = -1;
		root.directory_table[root.next].isEmpty = 0;

		strcpy(root.directory_table[root.next].name, name);

		root.directory_table[root.next].fat_index = FAT.next;
		root.directory_table[root.next].size = 0;
		root.directory_table[root.next].creation_time = time(NULL);

		fdt[fileID].root_index = root.next;


		write_blocks( 0, 1, (void *)&root );
		write_blocks( 1, 1, (void *)&FAT );
		write_blocks(DISKSIZE-1, 1, (void *)&freeList);
	}
	else								// Existing file
	{
		fdt[fileID].write_ptr = root.directory_table[fileIndex].size;
	}

	return fileID;
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



int getFileIndex(char* name)
{
	int i;
	for(i = 0; i < MAXIMUM_FILES; i++)
	{
		if(strcmp(name, root.directory_table[i].name) == 0)
		{
			return i;
		}
	}

	return -1;
}


int getNextFreeBlock()
{
	int i;
	for(i = 2; i < DISKSIZE - 1; i++)
	{
		if (freeList.freeblocks[i] == 0)
		{
			freeList.freeblocks[i] = 1;
			return i;
		}
	}

	printf("WARNING DISK IS FULL\n");
	printf("next free block: %d\n", i);

	return -1;
}

int getNextFatIndex()
{
    FAT.next += 1;

    if (FAT.next > DISKSIZE - 1)
    {
        int i;
        for (i = 0; i < DISKSIZE; i++)
        {
            if (FAT.fatNodes[i].index == EMPTY)
            {
                break;
            }
        }

        FAT.next = i;
        if (i == DISKSIZE)
        {
            printf("WARNING DISK IS FULL\n");
            return -1;
        }
    }

    return 0;
}

int getNextRootIndex()
{
    root.next += 1;

    if (root.next > MAXIMUM_FILES - 1)
    {
        int i;
        for (i = 0; i < MAXIMUM_FILES; i++)
        {
            if (root.directory_table[i].isEmpty == 1)
            {
                break;
            }
        }

        root.next = i;
        if (i == MAXIMUM_FILES)
        {
            printf("WARNING MAXIMUM FILE CAPACITY REACHED\n");
            return -1;
        }
    }

    return 0;
}
