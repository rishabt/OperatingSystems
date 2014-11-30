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
    diskFile dir_table[MAXIMUM_FILES];
    int next;
} rootDirectory;

typedef struct fatNode
{
    int db_index;
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
    //int fileId;   // The index in the array of the fdt acts as the fileId
    int root_index;
    //fat_node root_fat;
    int opened;
    int write_ptr;
    int read_ptr;
} fdtNode;

typedef struct freeblocks
{
    int freeblocks[DISKSIZE];
} freeblocks;

int mksfs(int fresh);

void sfs_ls(void);

int sfs_fopen(char *name);

int sfs_fclose(int fileID);

int sfs_fwrite(int fileID, char *buf, int length);

int sfs_fread(int fileID, char *buf, int length);

int sfs_fseek(int fileID, int offset);

int sfs_remove(char *file);
