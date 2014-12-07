#include "disk_emu.h"
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef _HEADER_
#define _HEADER_

#define BLOCK 0
#define FREELIST 1
#define ROOTLOCATION 2
#define SIZE_ROOT 20
#define FATLOCATION ROOTLOCATION + SIZE_ROOT
#define SIZE_FAT 4
#define STARTDATA FATLOCATION + SIZE_FAT
#define BLOCK_SIZE 2048

#define NUMBER_BLOCKS 1 + BLOCK_SIZE + SIZE_ROOT + SIZE_FAT

#define MAXNAME 30
#define FILENAME "root.sfs"

typedef struct filesystem_entry {
    char filename[MAXNAME + 1];
    unsigned short index;
    unsigned int size;
    time_t creationTime;
} filesystem_entry;

typedef struct FAT_Node {
    unsigned short stored_data;
    unsigned short next;
} FAT_Node;

typedef struct descriptor {
    unsigned int rd_ptr;
    unsigned int wt_ptr;
    unsigned int size;
    unsigned short ptr_start;
} descriptor;

int open_files;
filesystem_entry *root;
descriptor **fdt;
FAT_Node *FAT;

void mksfs(int fresh);

void sfs_ls(void);

int sfs_fopen(char *name);

int sfs_fclose(int fileID);

int sfs_fwrite(int fileID, char *buf, int length);

int sfs_fread(int fileID, char *buf, int length);

int sfs_fseek(int fileID, int offset);

int sfs_remove(char *file);

int getFirstUnusedSpot();

void allocateIndex(unsigned short indx);

void freeIndex(unsigned short indx);

#endif
