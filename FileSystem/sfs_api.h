#include "disk_emu.h"
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>

#define DISKSIZE 1000
#define MAXIMUM_FILES 100
#define EMPTY (-2)

typedef struct diskFile
{
    char name[32];
    int fat_index;
    time_t creation_time;
    int size;           // In bytes
    int isEmpty;
} diskFile;

typedef struct rootDirectory
{
    diskFile directory_table[MAXIMUM_FILES];
    int next;
} rootDirectory;

typedef struct fatNode
{
    int index;
    int next;
} fatNode;

typedef struct fat
{
    fatNode fatNodes[DISKSIZE];
    int next;
} fat;

typedef struct fdtNode
{
    char filename[32];
    int root_index;
    int opened;
    int write_ptr;
    int read_ptr;
} fdtNode;

typedef struct freeblocks
{
    int freeblocks[DISKSIZE];
} freeblocks;


rootDirectory root;
fat FAT;
freeblocks freeList;
fdtNode fdt[MAXIMUM_FILES];

static int opened_files = 0;

static int fsSize;
static int fatSize;
static int BLOCKSIZE;


void mksfs(int fresh);

void sfs_ls(void);

int sfs_fopen(char *name);

int sfs_fclose(int fileID);

int sfs_fwrite(int fileID, char *buf, int length);

int sfs_fread(int fileID, char *buf, int length);

int sfs_fseek(int fileID, int offset);

int sfs_remove(char *file);

int getFileIndex(char* name);

int getNextFreeBlock();

int getNextFatIndex();

int getNextRootIndex();

int isFileOpen(char* name);

int sizeInBlock(int size);

int getReadBlock(int size);
