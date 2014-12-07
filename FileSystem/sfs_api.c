#include "sfs_api.h"
#include "disk_emu.h"
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

void mksfs(int fresh)
{
    if (fresh)						// Check if a fresh disk requested
    {
        if(access(FILENAME, F_OK) != -1)		// Deletes the existing fs with same filename
        {
            unlink(FILENAME);
        }

        if (init_fresh_disk(FILENAME, BLOCK_SIZE, NUMBER_BLOCKS) != 0)		//Initialize new disk
        {
            fprintf(stderr, "Error creating filesystem");
        }

        int *storage = malloc(BLOCK_SIZE);				// Create storage block

        if (!storage)
        {
            fprintf(stderr, "Error creating storage block");
        }

        storage[0] = BLOCK_SIZE;
        storage[1] = NUMBER_BLOCKS;
        storage[1] = FREELIST;
        storage[2] = ROOTLOCATION;
        storage[4] = SIZE_ROOT;
        storage[5] = FATLOCATION;
        storage[6] = SIZE_FAT;
        storage[7] = STARTDATA;

        write_blocks(BLOCK, 1, storage);

        free(storage);			// Free memory

        unsigned int *buf_free = malloc(BLOCK_SIZE);		// Free sector list

        if (!buf_free)
        {
            fprintf(stderr, "Error in initializing free sector list");
        }

        int i;

        for (i = 0; i < BLOCK_SIZE/sizeof(unsigned int); i++)		//1 ->available; 0 -> unavailable
        {
            buf_free[i] = ~0;
        }

        write_blocks(FREELIST, 1, buf_free);
        free(buf_free);

        filesystem_entry *root_entry = malloc(BLOCK_SIZE*sizeof(filesystem_entry));	// root directory created

        if (!root_entry)
        {
            fprintf(stderr, "Error in initializing root");
        }

        for (i = 0; i < BLOCK_SIZE*SIZE_ROOT/sizeof(filesystem_entry); i++)
            root_entry[i] = (filesystem_entry) {.filename = "\0", .size=0, .index = BLOCK_SIZE };

        write_blocks(ROOTLOCATION, SIZE_ROOT, root_entry);
        free(root_entry);

        FAT_Node *fat_new = malloc(BLOCK_SIZE * sizeof(FAT_Node));			// Initialize a file allocation table

        if (!fat_new)
        {
            fprintf(stderr, "Error in intitializing FAT");
        }

        for (i = 0; i < BLOCK_SIZE; i++)
        {
            fat_new[i].stored_data = BLOCK_SIZE;
            fat_new[i].next = BLOCK_SIZE;
        }

        write_blocks(FATLOCATION, SIZE_FAT, fat_new);
        free(fat_new);

    }
    else		// if not a fresh disk
    {
        if (init_disk(FILENAME, BLOCK_SIZE, NUMBER_BLOCKS) != 0)		// open existing disk
        {
            fprintf(stderr, "Error in opening disk");
        }
    }

    int *super = malloc(BLOCK_SIZE);

    if (!super)
    {
        fprintf(stderr, "Error reading super block");
    }

    read_blocks(BLOCK, 1, super);

    open_files = 0;

    root = malloc(sizeof(filesystem_entry) * BLOCK_SIZE);
    FAT = malloc(sizeof(FAT_Node) * BLOCK_SIZE);

    if (!root || !FAT)
    {
        fprintf(stderr, "malloc error");
        exit(1);}

    read_blocks(ROOTLOCATION, SIZE_ROOT, root);
    read_blocks(FATLOCATION, SIZE_FAT, FAT);
}

void sfs_ls(void)
{
    if (!root)			// Check if root initialized
    {
        fprintf(stderr, "File system needs to be initialized first");
        return;
    }

    int k;

    for (k = 0; k < BLOCK_SIZE; k++)
    {
        if (strncmp(root[k].filename, "\0", 1) != 0)
        {
            printf("%12s %dKB %s", root[k].filename, root[k].size, ctime(&root[k].creationTime));
        }
    }
}

int sfs_fopen(char *name)
{
    if (strlen(name) > MAXNAME)		// Valid name check
    {
        return -1;
    }

    if (!root)		// Check root
    {
        fprintf(stderr,
            "Error in sfs_fopen.\nFile system neads to be opened first");
        return -1;
    }

    int k;

    for (k = 0; k < BLOCK_SIZE; k++)
    {
        if (strcmp(root[k].filename, name) == 0)
        {
            int tmp = -1, j;


            for (j = 0; j < open_files; j++)
            {
                if (fdt[j] && root[k].index == fdt[j]->ptr_start)
                {
                    return j;
                }
            }

            for (j = 0; j < open_files; j++)
            {
                if (!fdt[j])
                {
                    fdt[j] = malloc(sizeof(descriptor));
                    tmp = j;
                    break;
                }
            }
            root[k].creationTime = time(NULL);

            if (tmp == -1)
            {
                fdt = realloc(fdt, (1+open_files)*(sizeof(descriptor *)));
                fdt[open_files] = (descriptor *) malloc(sizeof(descriptor));
                tmp = open_files++;
            }

            descriptor *correct = fdt[tmp];		// File Descriptor

            if (!correct)
            {
                fprintf(stderr, "Error opening %12s", name);
                return -1;}

            correct->rd_ptr = 0;
            correct->wt_ptr = root[k].size;
            correct->size = root[k].size;
            correct->ptr_start = root[k].index;
            return tmp;
        }
    }

    for (k = 0; k < BLOCK_SIZE; k++)
    {

        if (strncmp(root[k].filename, "\0", 1) == 0)
        {

            int tmp = -1, j;

            for (j = 0; j < open_files; j++)
            {
                if (fdt[j] == NULL){
                    fdt[j] = malloc(sizeof(descriptor));
                    tmp = j;
                    break;
                }
            }

            if (tmp == -1)
            {
                fdt = realloc(fdt, (1+open_files)*(sizeof(descriptor *)));
                fdt[open_files] = (descriptor *) malloc(sizeof(descriptor));
                tmp = open_files++;
            }

            descriptor *desc = fdt[tmp];

            if (!desc)
            {
                fprintf(stderr, "Error creating %12s", name);
                return -1;
            }

            int count = -1;

            for (k = 0; k < BLOCK_SIZE;k++)
            {
                if (FAT[k].stored_data == BLOCK_SIZE)
                {
                	count = k;
                    break;
                }
            }

            if (count == -1)
            {
            	return -1;
            }

            int data = getFirstUnusedSpot();
            if (data == -1) return -1;
            allocateIndex(data);

            desc->rd_ptr = 0;
            desc->wt_ptr = 0;
            desc->size = 0;
            desc->ptr_start = count;

            strncpy(root[k].filename, name, 13);
            root[k].size = 0;
            root[k].index = count;
            write_blocks(ROOTLOCATION, SIZE_ROOT, root);

            FAT[count].stored_data = data;
            write_blocks(FATLOCATION, SIZE_FAT, FAT);
            return tmp;
        }
    }


    return -1;
}

int sfs_fclose(int fileID)
{
    if (fileID >= open_files || fdt[fileID] == NULL)		//Check if fileID is valid and if file is open
    {
        return -1;
    }

    free(fdt[fileID]);
    fdt[fileID] = NULL;

    return 0;
}

int sfs_fwrite(int fileID, char *buf, int length)
{
    int len = length;

    if (buf == NULL || length < 0 ||
        fileID >= open_files || fdt[fileID] == NULL)
        return -1;

    descriptor *to_write = fdt[fileID];

    FAT_Node *current = &(FAT[to_write->ptr_start]);

    char *disk_buff = malloc(BLOCK_SIZE);
    int i = (to_write->wt_ptr) / BLOCK_SIZE;
    int j = (to_write->wt_ptr) % BLOCK_SIZE;

    while (i > 0)
    {
        current = &(FAT[current->next]);
        i--;}

    i = 0;

    int offset = 0;
    while (length > 0)
    {
        read_blocks(STARTDATA + current->stored_data, 1, disk_buff);
        memcpy(disk_buff + j, buf + offset, (BLOCK_SIZE - j < length ? BLOCK_SIZE - j :  length));

        write_blocks(STARTDATA + current->stored_data, 1, disk_buff);

        length -= (BLOCK_SIZE - j);
        offset += (BLOCK_SIZE - j);

        j = 0;
        i++;

        if (current->next == BLOCK_SIZE && length > 0)
        {

            int k;
            int success = 0;

            for (k = 0; k < BLOCK_SIZE; k++)
            {
                if (FAT[k].stored_data == BLOCK_SIZE)
                {
                    int nxt = getFirstUnusedSpot();
                    if (nxt == -1) return -1;
                    allocateIndex(nxt);

                    current->next = k;
                    FAT[k].stored_data = nxt;

                    write_blocks(FATLOCATION, SIZE_FAT, FAT);

                    success = 1;

                    break;
                }
            }
            if (!success)
                return -1;
        }

        if (length > 0)
            current = &(FAT[current->next]);
    }

    to_write->size = to_write->wt_ptr + len > to_write->size
                        ? to_write->wt_ptr + len
                        : to_write->size;

    to_write->wt_ptr += len;

    for (i = 0; i < BLOCK_SIZE; i++)
    {
        if (to_write->ptr_start == root[i].index)
        {
            root[i].size = to_write->size;
            write_blocks(ROOTLOCATION, SIZE_ROOT, root);
            break;
        }
    }

    free(disk_buff);
    return len;
}

int sfs_fread(int fileID, char *buf, int length)
{
    if (length < 0 || buf == NULL ||
        fileID >= open_files || fdt[fileID] == NULL)
        return -1;

    descriptor *to_read = fdt[fileID];

    FAT_Node *curr = &(FAT[to_read->ptr_start]);

    if (to_read->rd_ptr + length > to_read->size)
        length = to_read->size - to_read->rd_ptr;

    int len = length;
    char *temp = malloc(BLOCK_SIZE);
    int i = (to_read->rd_ptr) / BLOCK_SIZE;
    int j = (to_read->rd_ptr) % BLOCK_SIZE;

    while (i > 0)
    {
    	curr = &(FAT[curr->next]);
        i--;
    }

    int offset = 0;
    while (length > 0)
    {
        read_blocks(STARTDATA + curr->stored_data, 1, temp);

        memcpy(buf + offset, temp + j, (BLOCK_SIZE - j < length ? BLOCK_SIZE - j :  length));

        length -= (BLOCK_SIZE - j);

        offset += (BLOCK_SIZE - j);

        j = 0;

        if (curr->next == BLOCK_SIZE && length > 0)
        {
            return -1;
        }

        if (length > 0)
        {
        	curr = &(FAT[curr->next]);
        }
    }

    free(temp);
    to_read->rd_ptr += len;

    return len;
}

int sfs_fseek(int fileID, int offset)
{
    if (fileID >= open_files || fdt[fileID] == NULL)
    {
        return -1;
    }

    fdt[fileID]->rd_ptr = offset;
    fdt[fileID]->wt_ptr = offset;

    return 0;
}

int sfs_remove(char *file)
{
	int k;
    for (k = 0; k < BLOCK_SIZE; k++)
    {
        if (strncmp(root[k].filename, file, 13) == 0)
        {
        	filesystem_entry *rm = &(root[k]);

            strcpy(rm->filename,"\0");
            rm->size =0;
            FAT_Node *fat_tmp = &(FAT[rm->index]);
            rm->index = BLOCK_SIZE;

            while (1)
            {
                freeIndex(fat_tmp->stored_data);
                fat_tmp->stored_data = BLOCK_SIZE;
                if (fat_tmp->next == BLOCK_SIZE)
                {
                    break;

                }

                FAT_Node *fat_next = &(FAT[fat_tmp->next]);
                fat_tmp->next = BLOCK_SIZE;
                fat_tmp = fat_next;

            }

            write_blocks(FATLOCATION, SIZE_FAT, FAT);
            return 0;

        }
    }

    return -1;
}

int getFirstUnusedSpot()
{
    unsigned int *tmp = malloc(BLOCK_SIZE);

    if (!tmp){
        fprintf(stderr, "Malloc failed in 'first_open'\n");
        return -1;}

    read_blocks(FREELIST, 1, tmp);

    int i;
    for (i = 0; i < BLOCK_SIZE/sizeof(unsigned int); i++)
    {
        int p = ffs(tmp[i]);

        if (p)
        {
            return p + i*8*sizeof(unsigned int) -1;
        }
    }

    return -1;
}

void allocateIndex(unsigned short indx)
{
    int a = indx/(8*sizeof(unsigned int));

    int b = indx % (8*sizeof(unsigned int));

    unsigned int *tmp = malloc(BLOCK_SIZE);

    if (!tmp)
    {
        fprintf(stderr, "Malloc failed in 'set'\n");
        return;
    }

    read_blocks(FREELIST, 1, tmp);
    tmp[a] &= ~(1 << b);

    write_blocks(FREELIST, 1, tmp);

}

void freeIndex(unsigned short indx)
{
    int a = indx/(8*sizeof(unsigned int));

    int b = indx % (8*sizeof(unsigned int));

    unsigned int *tmp = malloc(BLOCK_SIZE);

    if (!tmp)
    {
        fprintf(stderr, "Malloc failed in 'clear'\n");
        return;
    }

    read_blocks(FREELIST, 1, tmp);
    tmp[a] |= 1 << b;

    write_blocks(FREELIST, 1, tmp);
}
