#ifndef FILE_SYSTEM_H
#define FILE_SYSTEM_H

#include <type.h>

/* Mem size */
// To simplify implementation:
// 1 Block = 1 Sector = 512 B 
// (Only in this lab...)
#define SECTOR_SIZE 512
#define INODE_SIZE sizeof(inode_t)
#define DENTRY_SIZE sizeof(dentry_t)
#define INODE_PER_SECTOR (SECTOR_SIZE / INODE_SIZE)


/* Mem Layout */

#define SUPERBLOCK_ADDR       0x5d000000    // 512 B
#define INODE_MAP_ADDR        0x5d000200    // 512 B
#define DATA_MAP_ADDR         0x5d000400    // 128 KB
#define INODE_BLOCK_ADDR      0x5d020400    // 512 KB
#define DATABLOCK_ADDR        0x5d0a0400    // 512 MB

/* File Name */
#define MAX_NAME_LENGTH 20

/* File System Parameter */
#define MAGIC_NUM        0x666666    // Magic number
#define FS_SIZE          1049346     // Number of Sectors
#define FS_START         1048576     // Start at 512 MB

#define IMAP_NUM         1
#define IMAP_OFFSET      1

#define DMAP_NUM         256
#define DMAP_OFFSET      2

#define INODE_NUM        512
#define INODE_OFFSET     258

#define DATABLOCK_NUM    1048576     // 1M
#define DATABLOCK_OFFSET 770

/* INODE */
// INO of root directory
#define ROOTINO        0
// Student ID: 2019K8009929030
#define NUM_DIR_PTR    5
#define NUM_INDIR1_PTR 3
#define NUM_INDIR2_PTR 2
#define NUM_INDIR3_PTR 1
// Type
#define FILE     0
#define DIR      1
// Access
#define R              0
#define W              1
#define RW             2
// V for FREEDOM!!!!!!!
#define FREE           5

/* LSEEK */
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2


// Max number of open files
#define NOFILE 16

// Max file size of each level
// (blocks)
#define PTR_PER_SECTOR 		(SECTOR_SIZE / 4)
#define MAX_SIZE_DIR		NUM_DIR_PTR
#define MAX_SIZE_INDIR1		(PTR_PER_SECTOR * NUM_INDIR1_PTR + MAX_SIZE_DIR)
#define MAX_SIZE_INDIR2		(PTR_PER_SECTOR * PTR_PER_SECTOR * NUM_INDIR2_PTR + MAX_SIZE_INDIR1)
#define MAX_SIZE_INDIR3		(PTR_PER_SECTOR * PTR_PER_SECTOR * PTR_PER_SECTOR * NUM_INDIR3_PTR + MAX_SIZE_INDIR2)



typedef struct superblock {
    uint32_t valid;             // 1 - FS initialized
	uint32_t magic;
	uint32_t fs_size;
	uint32_t fs_start;

	uint32_t imap_num;
	uint32_t imap_offset;

	uint32_t dmap_num;
	uint32_t dmap_offset;

	uint32_t inode_num;
	uint32_t inode_offset;

	uint32_t datablock_num;
	uint32_t datablock_offset;
} superblock_t;

typedef struct inode {
	uint8_t ino;
	uint8_t type;	   		// FILE/DIR
	uint8_t access;    		// R/W
	uint8_t block_num; 		// Data blocks used
	uint32_t used_size;     // Size of file/dir
	uint32_t dentry_num;    // Used by DIR
	uint32_t links;			// Number of links

    // Ptrs
	uint32_t dir_ptr[NUM_DIR_PTR];
	uint32_t indir1_ptr[NUM_INDIR1_PTR];
	uint32_t indir2_ptr[NUM_INDIR2_PTR];
    uint32_t indir3_ptr[NUM_INDIR3_PTR];
} inode_t;

typedef struct dentry {
	uint32_t ino;
	uint32_t type;		// FILE/DIR
	char name[MAX_NAME_LENGTH];
} dentry_t;

typedef struct fd {      
    uint32_t ino; 
    uint8_t  access;
    uint32_t read_point;
    uint32_t write_point;
}fd_t;

fd_t FD_table[NOFILE]; 

void set_bitmap(int i, uint8_t *bitmap);
int check_fs();
void set_imap(int id);
void set_dmap(int id);
void clear_imap(int id);
void clear_dmap(int id);
void write_inode_sector(uint32_t ino);
void clear_sector(uint8_t *sec);
uint32_t alloc_datablock();
void init_dentry(uint32_t datablock_id, uint32_t cur_ino, uint32_t parent_ino);
void do_mkfs();
void do_statfs();
uint32_t alloc_inode();
inode_t *get_inode(uint32_t ino);
void clear_inode(uint32_t ino);
void clear_datablock(uint32_t datablock_id);
void set_file_block(inode_t *file_inode, int file_datablock_id, int datablock_id);
void rw_datablock(inode_t *file_inode, uint32_t file_datablock_id, int op);
inode_t *search_dir(inode_t *dir, char *name, int type);
inode_t *find_path(char *path);
void do_mkdir(uintptr_t dirname);
void do_rmdir(uintptr_t dirname);
void do_cd(uintptr_t dirname);
void list_dentry(inode_t *dir_inode);
void do_ls(uintptr_t dirname);
int alloc_fd();
void do_touch(uintptr_t filename);
void do_cat(uintptr_t filename);
int do_file_open(uintptr_t filename, int access);
void do_file_close(int fd);
int do_file_read(int fd, uintptr_t buff, int size);
int do_file_write(int fd, uintptr_t buff, int size);
void do_ln(uintptr_t source, uintptr_t link_name);
void do_rm(uintptr_t filename);
void do_lseek(int fd, int offset, int whence);


#endif