#include "sfs_api.h"


void mksfs(int fresh)
{
	fsSize     = sizeof(rootDirectory);
	fatSize     = sizeof(fat);
	BLOCKSIZE   = ( fsSize > fatSize ? fsSize : fatSize );

	if(fresh)
	{
		// Initialize fresh disk
		init_fresh_disk("root_dir.sfs", BLOCKSIZE, DISKSIZE);

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
	else
	{
		// Initialize disk
		init_disk("root_dir.sfs", BLOCKSIZE, DISKSIZE);

		// Since it's not a fresh file system, read existing blocks
		read_blocks(0, 1, (void *)&root);
		read_blocks(1, 1, (void *)&FAT);
		read_blocks(DISKSIZE-1, 1, (void *)&freeList);

	}
}

void sfs_ls(void)
{
	int i = 0;

	while(i < MAXIMUM_FILES)
	{
		if(root.directory_table[i].isEmpty == 0)
		{
			printf("%s  %dKB  %s", root.directory_table[i].name, (root.directory_table[i].size/1000),
							ctime(&root.directory_table[i].creation_time));
		}
		i++;
	}

	printf("FILES %d", i);
	// Print name, size (in KB, divide by 1000) and time (convert to c-string to make it readable)
//	for(i = 0; root.directory_table[i].isEmpty == 0; i++)
//	{
//		printf("%s  %dKB  %s", root.directory_table[i].name, (root.directory_table[i].size/1000),
//				ctime(&root.directory_table[i].creation_time));
//
//	}
}

int sfs_fopen(char *name)
{
	int fileID = isFileOpen(name);

	if(fileID != -1)
	{
		return fileID;
	}
	printf("HERE 1\n");
	int fileIndex = getFileIndex(name);

	strcpy(fdt[opened_files].filename, name);
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

		if (getNextFatIndex() == -1)
		{
			exit(1);
		}

		if (getNextRootIndex() == -1)
		{
			exit(1);
		}

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
	if (opened_files <= fileID)
	{
		fprintf(stderr, "File does not exist %d\n", fileID);
		return -1;
	}
	else
	{
		fdt[ fileID ].opened = 0;
		return 1;
	}

	return -1;
}

int sfs_fwrite(int fileID, char *buf, int length)
{
	if(fileID <= opened_files)
	{
		fprintf(stderr, "File does not exist %d\n", fileID);
		return -1;
	}

	int len = length;

	int root_index = fdt[fileID].root_index;
	int fat_index = root.directory_table[root_index].fat_index;
	int index = FAT.fatNodes[fat_index].index;

	while(FAT.fatNodes[fat_index].next != -1)
	{
		index = FAT.fatNodes[fat_index].index;
		fat_index = FAT.fatNodes[fat_index].next;
	}

	char temp[BLOCKSIZE];

	read_blocks(index, 1, (void *)temp);

	int write_pointer = sizeInBlock(fdt[fileID].write_ptr);

	if (write_pointer != -1)
	{
		memcpy((temp + write_pointer), buf, (BLOCKSIZE - write_pointer));

		write_blocks(index, 1, (void *)temp);
		length = length - (BLOCKSIZE - write_pointer);
		buf = buf + (BLOCKSIZE - write_pointer);
	}

	while (length > 0)
	{
		memcpy( temp, buf, BLOCKSIZE );

		index = getNextFreeBlock();

		FAT.fatNodes[fat_index].next = FAT.next;
		FAT.fatNodes[FAT.next].index = index;
		fat_index = FAT.next;
		FAT.fatNodes[fat_index].next = -1;
		getNextFatIndex();

		length = length - BLOCKSIZE;
		buf = buf + BLOCKSIZE;

		write_blocks(index, 1, (void *)temp);
	}

	root.directory_table[root_index].size += len;
	fdt[fileID].write_ptr = root.directory_table[root_index].size;

	write_blocks(0, 1, (void *)&root);
	write_blocks(1, 1, (void *)&FAT);
	write_blocks(DISKSIZE-1, 1, (void *)&freeList);

	return 1;
}

int sfs_fread(int fileID, char *buf, int length)
{
	if (opened_files <= fileID && fdt[ fileID ].opened == 0 )
	{
		fprintf(stderr, "No file with ID %d is opened\n", fileID);
		return -1;
	}

	char* pointer = buf;

	char temp[BLOCKSIZE];

	int root_index = fdt[fileID].root_index;
	int fat_index = root.directory_table[root_index].fat_index;

	int index;
	index = FAT.fatNodes[fat_index].index;

	int read_pointer = sizeInBlock(fdt[ fileID ].read_ptr);

	int read_block_pointer = getReadBlock(fdt[ fileID ].read_ptr);

	while (read_block_pointer > 0)
	{
		if (FAT.fatNodes[fat_index].next != -1)
		{
			fat_index = FAT.fatNodes[fat_index].next;
			--read_block_pointer;
		}
	}

	index = FAT.fatNodes[fat_index].index;

	read_blocks(index, 1, temp);
	memcpy(pointer, (temp + read_pointer), (BLOCKSIZE - read_pointer));

	length = length - (BLOCKSIZE - read_pointer);
	pointer = pointer + (BLOCKSIZE - read_pointer);

	fat_index = FAT.fatNodes[fat_index].next;

	while((FAT.fatNodes[fat_index].next != -1) && (length > 0) && (length > BLOCKSIZE)) {
		index = FAT.fatNodes[fat_index].index;
		read_blocks(index, 1, temp);

		memcpy(pointer, temp, BLOCKSIZE);

		length -= BLOCKSIZE;
		pointer = pointer + BLOCKSIZE;
		fat_index = FAT.fatNodes[fat_index].next;
	}

	index = FAT.fatNodes[fat_index].index;
	read_blocks(index, 1, temp);

	memcpy(pointer, temp, length);
	fdt[fileID].read_ptr += length;

	return 1;
}

int sfs_fseek(int fileID, int offset)
{
	if (opened_files <= fileID)
	{
		fprintf(stderr, "No file with ID %d is opened\n", fileID);
		return -1;
	}

	fdt[fileID].write_ptr = offset;
	fdt[fileID].read_ptr = offset;

	return 1;
}

int sfs_remove(char *file)
{
	int temp_fat_index;
	int root_index = getFileIndex(file);
	if (root_index == -1)
	{
		return -1;
	}

	int fat_index;

	root.directory_table[root_index].isEmpty = 1;
	fat_index = root.directory_table[root_index].fat_index;

	while(FAT.fatNodes[fat_index].next != EMPTY)
	{
		FAT.fatNodes[fat_index].index = EMPTY;
		temp_fat_index = FAT.fatNodes[fat_index].next;

		FAT.fatNodes[fat_index].next = EMPTY;
		fat_index = temp_fat_index;
	}

	int fdt_index = isFileOpen(file);
	if ( fdt_index != -1 )
	{
		fdt[ fdt_index ].opened = 0;
	}

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

int isFileOpen(char* name)
{
    int i;
    int result;

    for(i = 0; i < MAXIMUM_FILES; i++)
    {
        result = strcmp(fdt[i].filename, name);

        if ( result == 0 )
        {
            return i;
        }
    }

    return -1;
}

int sizeInBlock(int size)
{
    if (size == 0)
    {
    	return 0;
    }

    if (size % BLOCKSIZE == 0)
    {
        return -1;
    }

    while(size > BLOCKSIZE)
    {
        size -= BLOCKSIZE;
    }

    return size;
}

int getReadBlock(int size)
{
    int block = 0;

    while (size > BLOCKSIZE)
    {
        size -= BLOCKSIZE;
        ++block;
    }

    return block;
}
