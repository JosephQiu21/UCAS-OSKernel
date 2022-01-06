#include <os/file.h>
#include <sbi.h>
#include <os/stdio.h>
#include <screen.h>
#include <os/time.h>
#include <os/string.h>
#include <pgtable.h>


inode_t *current_inode;

// Check if file system already exists on disk
int check_fs() {
    sbi_sd_read(SUPERBLOCK_ADDR, 1, FS_START);
    superblock_t *sb = (superblock_t *)(pa2kva(SUPERBLOCK_ADDR));
    if (sb -> magic != MAGIC_NUM) {
        prints("[ERROR] No File System!\n");
        return 0;
    } else {
        return 1;
    }
}

/* ---------- BITMAP SETTING ---------- */
/* Mem layout should be EXACTLY THE SAME as disk!!! */
/* But we can only read/write the blocks we need!!! */

/* We have to **first read map from disk**, then use these set functions!!! */

// Mark block `i` as **used** in imap
void set_imap(int id) {
    uint8_t *imap = (uint8_t *)(pa2kva(INODE_MAP_ADDR));
    imap[id / 8] = imap[id / 8] | (1 << (7 - (id % 8)));
    sbi_sd_write(INODE_MAP_ADDR, IMAP_NUM, FS_START + IMAP_OFFSET);
}


// Mark block `i` as **used** in dmap
void set_dmap(int id) {
    int dmap_sector_id = id / (SECTOR_SIZE * 8);
    uint8_t *dmap = (uint8_t *)(pa2kva(DATA_MAP_ADDR));
    dmap[id / 8] = dmap[id / 8] | (1 << (7 - (id % 8)));
    sbi_sd_write(DATA_MAP_ADDR + dmap_sector_id * SECTOR_SIZE, 1, FS_START + DMAP_OFFSET + dmap_sector_id);
}

/* We DON'T have to **first read map from disk**, then use these clear functions!!! */

// Mark block `i` as **free** in imap
void clear_imap(int id) {
    sbi_sd_read(INODE_MAP_ADDR, IMAP_NUM, FS_START + IMAP_OFFSET);
    uint8_t *imap = (uint8_t *)(pa2kva(INODE_MAP_ADDR));
    imap[id / 8] = imap[id / 8] & ((0xff - 1) << (7 - (id % 8)));
    sbi_sd_write(INODE_MAP_ADDR, IMAP_NUM, FS_START + IMAP_OFFSET);
}

// Mark block `i` as **free** in dmap
void clear_dmap(int id) {
    int dmap_sector_id = id / (SECTOR_SIZE * 8);
    sbi_sd_read(DATA_MAP_ADDR + dmap_sector_id * SECTOR_SIZE, 1, FS_START + DMAP_OFFSET + dmap_sector_id);
    uint8_t *dmap = (uint8_t *)(pa2kva(DATA_MAP_ADDR));
    dmap[id / 8] = dmap[id / 8] & ((0xff - 1) << (7 - (id % 8)));
    sbi_sd_write(DATA_MAP_ADDR + dmap_sector_id * SECTOR_SIZE, 1, FS_START + DMAP_OFFSET + dmap_sector_id);
}

/* ---------- SECTOR OPERATING ---------- */
// Write a sector containing inode `ino` into disk
void write_inode_sector(uint32_t ino) {
    uint32_t inode_sector_id = ino / INODE_PER_SECTOR;
    sbi_sd_write(INODE_BLOCK_ADDR + inode_sector_id * SECTOR_SIZE, 1, FS_START + INODE_OFFSET + inode_sector_id);
}

// Clear sector pointed by ptr `sec` in MEM
void clear_sector(uint8_t *sec) {
    kmemset(sec, 0, 512);
}

// Allocate one free data block using data map
// Return data block **ID**
uint32_t alloc_datablock() {
    // Look for a free block
    int free_block;
    for (int s = 0; s < DMAP_NUM; s++) {
        // Read data map from disk
        sbi_sd_read(DATA_MAP_ADDR + SECTOR_SIZE * s, 1, FS_START + DMAP_OFFSET + s);
        uint8_t *dmap = (uint8_t *)(pa2kva(DATA_MAP_ADDR));

        for (int i = 0; i < SECTOR_SIZE; i++) {
            for (int j = 0; j < 8; j++) {
                if (!(dmap[i] & (0x80 >> j))) {
                free_block = SECTOR_SIZE * 8 * s + 8 * i + j;
                set_dmap(free_block);
                return free_block;
            }
            }
        }
    }
}

// Init `.` and `..` for a directory
void init_dentry(uint32_t datablock_id, uint32_t cur_ino, uint32_t parent_ino) {
    dentry_t *dentry = (dentry_t *)(pa2kva(DATABLOCK_ADDR));
    clear_sector(dentry);
    dentry[0].ino = cur_ino;
    dentry[0].type = DIR;
    kstrcpy(dentry[0].name, ".");
    dentry[1].ino = parent_ino;
    dentry[1].type = DIR;
    kstrcpy(dentry[1].name, "..");
    sbi_sd_write(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + datablock_id);
}

/* ---------- FILE SYSTEM INITIALIZING ---------- */

// Make file system
void do_mkfs() {
    sbi_sd_read(SUPERBLOCK_ADDR, 1, FS_START);
    superblock_t *sb = (superblock_t *)(pa2kva(SUPERBLOCK_ADDR));

    /* See if the fs already exists */
    if (sb -> magic == MAGIC_NUM) {
        prints("[FS] FS already exists in disk!\n");
        sbi_sd_read(INODE_BLOCK_ADDR, 1, FS_START + INODE_OFFSET);
        current_inode = (inode_t *)(pa2kva(INODE_BLOCK_ADDR));
        statfs();
        return;
    } else {

        /* Init Superblock */
        prints("[FS] Start initializing!\n");
        prints("[FS] Initializing super block...\n");
        sb -> magic = MAGIC_NUM;
        sb -> fs_size = FS_SIZE;
        sb -> fs_start = FS_START;
        sb -> imap_num = IMAP_NUM;
        sb -> imap_offset = IMAP_OFFSET;
        sb -> dmap_num = DMAP_NUM;
        sb -> dmap_offset = DMAP_OFFSET;
        sb -> inode_num = INODE_NUM;
        sb -> inode_offset = INODE_OFFSET;
        sb -> datablock_num = DATABLOCK_NUM;
        sb -> datablock_offset = DATABLOCK_OFFSET;

        prints("---------------- File System INFO ----------------\n");
        prints("MAGIC: %x\n", sb -> magic);
        prints("TOTAL SIZE: %d sectors,\n", sb -> fs_size);
        prints("Start at %d sectors on disk\n", sb -> fs_start);
        prints("INODE MAP ---- OFFSET: %d SIZE: %d\n", sb -> imap_offset, sb -> imap_num);
        prints("DATA MAP ----- OFFSET: %d SIZE: %d\n", sb -> dmap_offset, sb -> dmap_num);
        prints("INODE -------- OFFSET: %d SIZE: %d\n", sb -> inode_offset, sb -> inode_num);
        prints("DATA BLOCK --- OFFSET: %d SIZE: %d\n", sb -> datablock_offset, sb -> datablock_num);

        /* Init inode map */
        prints("[FS] Setting inode map...\n");
        uint8_t *imap = (uint8_t *)(pa2kva(INODE_MAP_ADDR));
        clear_sector(imap);
        set_imap(0);    // First inode is used as root dir

        /* Init data map */
        prints("[FS] Setting data map...\n");
        uint8_t *dmap = (uint8_t *)(pa2kva(DATA_MAP_ADDR));
        // Clear 256 sectors
        for (int i = 0; i < DMAP_NUM; i++) {
            clear_sector(dmap + SECTOR_SIZE * i);
        }

        // Write Superblock, inode map, and data map back to disk
        sbi_sd_write(SUPERBLOCK_ADDR, INODE_OFFSET, FS_START);

        /* Init root directory inode */
        prints("[FS] Setting inode...\n");
        inode_t *inode = (inode_t *)(pa2kva(INODE_BLOCK_ADDR));
        inode[0].ino = ROOTINO;
        inode[0].type = DIR;
        inode[0].access = RW;
        inode[0].block_num = 1;
        inode[0].dentry_num = 2;
        inode[0].used_size = 2 * sizeof(dentry_t);
        inode[0].dir_ptr[0] = alloc_datablock();

        // Initialize global current inode pointer
        current_inode = inode;
        // Write back to disk
        write_inode_sector(ROOTINO);

        /* Clear data block */
        // TODO: do we need it?

        /* Setting dentry */
        init_dentry(0, 0, 0);

        /* Set valid bit in superblock */
        sb -> valid = 1;

        prints("[FS] File system initialized!\n");
        screen_reflush();
    }
}

// Print file system info
void do_statfs() {
    sbi_sd_read(SUPERBLOCK_ADDR, 1, FS_START);
    superblock_t *sb = (superblock_t *)(pa2kva(SUPERBLOCK_ADDR));

    if (sb -> magic != MAGIC_NUM) {
        prints("[ERROR] NO file system!\n");
    }

    int used_data_block = 0, used_inode_block = 0;

    // Count number of used data blocks
    sbi_sd_read(DATA_MAP_ADDR, DMAP_NUM, FS_START + DMAP_OFFSET);
    uint8_t *dmap = (uint8_t *)(pa2kva(DATA_MAP_ADDR));
    for (int i = 0; i < sb -> dmap_num * 512 / 8; i++) {
        for (int j = 0; j < 8; j++) {
            used_data_block += (dmap[i] >> j) & 1;
        }
    }

    // Count number of used inode blocks
    sbi_sd_read(INODE_MAP_ADDR, IMAP_NUM, FS_START + IMAP_OFFSET);
    uint8_t *imap = (uint8_t *)(pa2kva(INODE_MAP_ADDR));
    for (int i = 0; i < sb -> imap_num * 512 / 8; i++) {
        for (int j = 0; j < 8; j++) {
            used_inode_block += (dmap[i] >> j) & 1;
        }
    }

    prints("---------------- File System INFO ----------------\n");
    prints("MAGIC: %x\n", sb -> magic);
    prints("USED DATA BLOCK: %d/%d, START SECTOR: %d (0x%x)\n", used_data_block, sb -> fs_size, sb -> fs_start, sb -> fs_start);
    prints("INODE MAP ---- OFFSET: %d SIZE: %d\n", sb -> imap_offset, sb -> imap_num);
    prints("DATA MAP ----- OFFSET: %d SIZE: %d\n", sb -> dmap_offset, sb -> dmap_num);
    prints("INODE -------- OFFSET: %d SIZE: %d\n", sb -> inode_offset, sb -> inode_num);
    prints("DATA BLOCK --- OFFSET: %d SIZE: %d\n", sb -> datablock_offset, sb -> datablock_num);
    prints("INODE SIZE: %dB, DIR ENTRY SIZE: %dB\n", INODE_SIZE, DENTRY_SIZE);
}

/* ---------- INODE OPERATING ---------- */

// Allocate a free inode using inode map
// Return ino
uint32_t alloc_inode() {
    // Read inode bitmap from disk
    sbi_sd_read(INODE_MAP_ADDR, IMAP_NUM, FS_START + IMAP_OFFSET);
    uint8_t *imap = (uint8_t *)(pa2kva(INODE_MAP_ADDR));

    // Look for a free inode
    int free_inode;
    for (int i = 0; i < IMAP_NUM * SECTOR_SIZE / 8; i++) {
        for (int j = 0; j < 8; j++) {
            if (!(imap[i] & (0x80 >> j))) {
                free_inode = 8 * i + j;
                set_imap(free_inode);
                return free_inode;
            }
        }
    }
}

// Get inode of `ino` (in-mem copy)
inode_t *get_inode(uint32_t ino) {
    uint32_t inode_block_id = ino / INODE_PER_SECTOR;
    uint32_t inode_block_offset = ino % INODE_PER_SECTOR;

    // Read the block containing inode `ino` from disk
    sbi_sd_read(INODE_BLOCK_ADDR + SECTOR_SIZE * inode_block_id, 1, FS_START + INODE_OFFSET + inode_block_id);

    inode_t *ref_inode_block = (inode_t *)(pa2kva(INODE_BLOCK_ADDR) + SECTOR_SIZE * inode_block_id);
    return &(ref_inode_block[inode_block_offset]);
}

// Clear inode `ino` on disk
void clear_inode(uint32_t ino) {
    uint32_t inode_block_id = ino / INODE_PER_SECTOR;
    uint32_t inode_block_offset = (ino % INODE_PER_SECTOR) * INODE_SIZE;

    // Read the block containing inode `ino` from disk
    sbi_sd_read(INODE_BLOCK_ADDR + SECTOR_SIZE * inode_block_id, 1, FS_START + INODE_OFFSET + inode_block_id);

    inode_t *inode_block = (inode_t *)(pa2kva(INODE_BLOCK_ADDR) + SECTOR_SIZE * inode_block_id);
    inode_t *inode = &(inode_block[inode_block_offset]);
    kmemset(inode, 0, INODE_SIZE);

    // Write back into disk
    sbi_sd_write(INODE_BLOCK_ADDR + SECTOR_SIZE * inode_block_id, 1, FS_START + INODE_OFFSET + inode_block_id);
}

/* ---------- DATA BLOCK OPERATING ---------- */
// Clear data block (id)
void clear_datablock(uint32_t datablock_id) {
    kmemset(DATABLOCK_ADDR, 0, SECTOR_SIZE);
    sbi_sd_write(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + datablock_id);
}

void set_file_block(inode_t *file_inode, int file_datablock_id, int datablock_id) {
    if (file_datablock_id < MAX_SIZE_DIR) {
        file_inode -> dir_ptr[file_datablock_id] = datablock_id;
    }
    // TODO: support large file

    // Write inode back into disk
    write_inode_sector(file_inode -> ino);
}

// Read/Write/Free ONE data block used by file
void rw_datablock(inode_t *file_inode, uint32_t file_datablock_id, int op) {

    char *file_data = (char *)(pa2kva(DATABLOCK_ADDR)); 
    char *indir1_block = (char *)(pa2kva(DATABLOCK_ADDR + SECTOR_SIZE));
    char *indir2_block = (char *)(pa2kva(DATABLOCK_ADDR + 2 * SECTOR_SIZE));
    char *indir3_block = (char *)(pa2kva(DATABLOCK_ADDR + 3 * SECTOR_SIZE));

    if (op == R || op == FREE) {
        clear_sector(file_data);
    } 

    if (file_datablock_id < MAX_SIZE_DIR) {
        if (op == R) {
            sbi_sd_read(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + file_inode -> dir_ptr[file_datablock_id]);
        }
        else if (op == W){
            sbi_sd_write(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + file_inode -> dir_ptr[file_datablock_id]);
        } 
        else {
            sbi_sd_write(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + file_inode -> dir_ptr[file_datablock_id]);
            clear_dmap(file_inode -> dir_ptr[file_datablock_id]);
        }
    }
    else if (file_datablock_id < MAX_SIZE_INDIR1) {
        int indir1_block_id = (file_datablock_id - MAX_SIZE_DIR) / PTR_PER_SECTOR;
        int indir1_block_offset = (file_datablock_id - MAX_SIZE_DIR) % PTR_PER_SECTOR;

        clear_sector(indir1_block);
        sbi_sd_read(DATABLOCK_ADDR + SECTOR_SIZE, 1, FS_START + DATABLOCK_OFFSET + file_inode -> indir1_ptr[indir1_block_id]);
        if (op == R) {
            sbi_sd_read(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + indir1_block[indir1_block_offset]);
        }
        else if (op == W ) {
            sbi_sd_write(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + indir1_block[indir1_block_offset]);
        }
        else {
            sbi_sd_write(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + indir1_block[indir1_block_offset]);
            clear_dmap(indir1_block[indir1_block_offset]);
        }
    }
    else if (file_datablock_id < MAX_SIZE_INDIR2) {
        int indir2_block_id = (file_datablock_id - MAX_SIZE_INDIR1) / PTR_PER_SECTOR;
        int indir2_block_offset = (file_datablock_id - MAX_SIZE_INDIR1) % PTR_PER_SECTOR;
        int indir1_block_id = indir2_block_id / PTR_PER_SECTOR;
        int indir1_block_offset = indir2_block_id % PTR_PER_SECTOR;
        clear_sector(indir1_block);
        sbi_sd_read(DATABLOCK_ADDR + SECTOR_SIZE, 1, FS_START + DATABLOCK_OFFSET + file_inode -> indir2_ptr[indir1_block_id]);
        clear_sector(indir2_block);
        sbi_sd_read(DATABLOCK_ADDR + 2 * SECTOR_SIZE, 1, FS_START + DATABLOCK_OFFSET + indir1_block[indir1_block_offset]);
        if (op == R) {
            sbi_sd_read(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + indir2_block[indir2_block_offset]);
        }
        else if (op == W ) {
            sbi_sd_write(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + indir2_block[indir2_block_offset]);
        }
        else {
            sbi_sd_write(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + indir2_block[indir2_block_offset]);
            clear_dmap(indir2_block[indir2_block_offset]);
        }
    }
    else {
        int indir3_block_id = (file_datablock_id - MAX_SIZE_INDIR2) / PTR_PER_SECTOR;
        int indir3_block_offset = (file_datablock_id - MAX_SIZE_INDIR2) % PTR_PER_SECTOR;
        int indir2_block_id = indir3_block_id / PTR_PER_SECTOR;
        int indir2_block_offset = indir3_block_id % PTR_PER_SECTOR;
        int indir1_block_id = indir2_block_id / PTR_PER_SECTOR;
        int indir1_block_offset = indir2_block_id % PTR_PER_SECTOR;
        clear_sector(indir1_block);
        sbi_sd_read(DATABLOCK_ADDR + SECTOR_SIZE, 1, FS_START + DATABLOCK_OFFSET + file_inode -> indir3_ptr[indir1_block_id]);
        clear_sector(indir2_block);
        sbi_sd_read(DATABLOCK_ADDR + 2 * SECTOR_SIZE, 1, FS_START + DATABLOCK_OFFSET + indir1_block[indir1_block_offset]);
        clear_sector(indir3_block);
        sbi_sd_read(DATABLOCK_ADDR + 3 * SECTOR_SIZE, 1, FS_START + DATABLOCK_OFFSET + indir2_block[indir2_block_offset]);
        if (op == R) {
            sbi_sd_read(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + indir3_block[indir3_block_offset]);
        }
        else if (op == W ) {
            sbi_sd_write(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + indir3_block[indir3_block_offset]);
        }
        else {
            sbi_sd_write(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + indir3_block[indir3_block_offset]);
            clear_dmap(indir3_block[indir3_block_offset]);
        }
    }
}


/* ---------- DIRECTORY OPERATING ---------- */

// Search for a dentry in a directory
// return ptr to inode (in-mem copy)
inode_t *search_dir(inode_t *dir, char *name, int type) {
    if (dir -> type != DIR) {
        prints("[ERROR] You are not searching in a directory!\n");
        return 0;
    }

    // Read datablock of `dir` into mem
    sbi_sd_read(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + dir -> dir_ptr[0]);
    dentry_t *dentry = (dentry_t *)(pa2kva(DATABLOCK_ADDR));
    for (int i = 0; i < dir -> dentry_num; i++, dentry++) {
        if (!strcmp(name, dentry -> name) && (dentry -> type == type)) {
            return get_inode(dentry -> ino);
        }
    }
    return 0;
}

// Path resolution (three layers at most)
// for example, `dir1/dir2/dir3`
// return pointer to inode (in-mem copy)
inode_t *find_path(char *path) {
    char dir[5][10] = {0};
    int i = 0, j = 0, k = 0;
    while (path[k] != '\0') {
        if (path[k] != '/') {
            dir[i][j++] = path[k];
        }
        else {
            dir[i++][j] = '\0';
            j = 0;
        }
        k++;
    }
    int level = i;  
    // 0 - search in current dir
    // 1 - search in child dir
    // 2 - search in child/grandchild dir

    if (dir[3][0] != 0) {
        prints("[ERROR] This file system does not support 3+ levels directory search.\n");
        return 0;
    }

    // Search current directory
    inode_t *dir1_inode = search_dir(current_inode, dir[0], DIR);
    if (dir1_inode == 0) {
        prints("[ERROR] Directory `%s` not found!", dir[0]);
        return 0;
    }
    if (level == 0) return dir1_inode;

    // Search dir1
    inode_t *dir2_inode = search_dir(dir1_inode, dir[1], DIR);
    if (dir2_inode == 0) {
        prints("[ERROR] Directory `%s` not found!", dir[1]);
        return 0;
    }
    if (level == 1) return dir2_inode;

    // Search dir2
    inode_t *dir3_inode = search_dir(dir2_inode, dir[2], DIR);
    if (dir3_inode == 0) {
        prints("[ERROR] Directory `%s` not found!", dir[2]);
        return 0;
    }
    if (level == 2) return dir3_inode;
}

// Create a directory
// 1. Initialize inode of new dir
// 2. Initialize data of new dir
// 3. Update data of current dir
// 4. Update inode of current dir
void do_mkdir(uintptr_t dirname) {
    // Check if FS exists
    if (!check_fs()) return;

    // We always use `current_inode` to refer to current dir
    // It is initialized as pointer to root dir
    inode_t *cur_dir_inode = current_inode;
    inode_t *new_dir_inode = search_dir(cur_dir_inode, (char *)dirname, DIR);

    if (new_dir_inode != 0 && new_dir_inode -> type == DIR) {
        prints("[ERROR] Directory has existed!\n");
        return;
    }

    uint32_t ino = alloc_inode();
    new_dir_inode = get_inode(ino);

    new_dir_inode -> ino = ino;
    new_dir_inode -> type = DIR;
    new_dir_inode -> access = RW;
    new_dir_inode -> block_num = 1;     // Directory uses only one block
    new_dir_inode -> used_size = 2 * sizeof(dentry_t);
    new_dir_inode -> dentry_num = 2;
    new_dir_inode -> dir_ptr[0] = alloc_datablock();

    write_inode_sector(ino);

    // 2. Initialize data of new dir
    // Create `.` and `..`
    init_dentry(new_dir_inode -> dir_ptr[0], new_dir_inode -> ino, cur_dir_inode -> ino);

    // 3. Update data of current dir
    // Add dentry in current directory
    sbi_sd_read(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + cur_dir_inode -> dir_ptr[0]);
    dentry_t *new_dentry = (dentry_t *)(pa2kva(DATABLOCK_ADDR) + cur_dir_inode -> used_size);
    new_dentry -> type = DIR;
    new_dentry -> ino = new_dir_inode -> ino;
    kstrcpy(new_dentry -> name, (char *)dirname);
    sbi_sd_write(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + cur_dir_inode -> dir_ptr[0]);

    // 4. Update inode of current dir
    cur_dir_inode -> dentry_num++;
    cur_dir_inode -> used_size += sizeof(dentry_t);
    write_inode_sector(cur_dir_inode -> ino);

    
}

// Remove a directory
// 1. Update data of current dir
// 2. Update inode of current dir
// 3. Free data of removed dir & Clear data bitmap
// 4. Free inode of removed dir & Clear inode bitmap
void do_rmdir(uintptr_t dirname) {
    // Check if FS exists
    if (!check_fs()) return;

    inode_t *cur_dir_inode = current_inode;
    inode_t *rm_dir_inode;
    if ((rm_dir_inode = search_dir(cur_dir_inode, (char *)dirname, DIR)) == 0) {
        prints("[ERROR] No such directory!\n");
        return;
    }

    // Require users to clear directory before removing
    if (rm_dir_inode -> dentry_num > 2) {
        prints("[ERROR] You have to clear the directory before removing it!\n");
        return;
    }

    // 1. Update data of current dir
    sbi_sd_read(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + cur_dir_inode -> dir_ptr[0]);
    dentry_t *dentry = (dentry_t *)(pa2kva(DATABLOCK_ADDR));
    for (int i = 0; i < cur_dir_inode -> dentry_num; i++) {
        if (dentry[i].ino == rm_dir_inode -> ino) {
            if (i != cur_dir_inode -> dentry_num - 1) {
                // Move dentry[i + 1] ~ dentry[dentry_num - 1] forward
                kmemcpy(&(dentry[i]), &(dentry[i + 1]), (cur_dir_inode -> dentry_num - 1 - i) * sizeof(dentry_t));
            } else {
                kmemset(&(dentry[i]), 0, sizeof(dentry_t));
            }
            break;
        }
    }
    sbi_sd_write(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + cur_dir_inode -> dir_ptr[0]);

    // 2. Update inode of current dir
    cur_dir_inode -> dentry_num--;
    cur_dir_inode -> used_size -= sizeof(dentry_t);
    write_inode_sector(cur_dir_inode -> ino);

    // 3. Free data of removed dir & Clear data bitmap
    clear_datablock(rm_dir_inode -> dir_ptr[0]);
    clear_dmap(rm_dir_inode -> dir_ptr[0]);

    // 4. Free inode of removed dir & Clear inode bitmap
    clear_imap(rm_dir_inode -> ino);
    clear_inode(rm_dir_inode -> ino);

    prints("[FS] Directory `%s` removed!\n", (char *)dirname);
}

// Change directory
// `cd dir1/dir2/dir3`
void do_cd(uintptr_t dirname) {
    // Check if FS exists
    if (!check_fs()) return;

    inode_t *target_dir_inode;

    char *path = (char *)dirname;
    if (path[0] == '\0') return;
    if ((target_dir_inode = find_path(path)) == 0) {
        prints("[ERROR] Fail to change directory!\n");
        return;
    }
    current_inode = target_dir_inode;
    prints("[FS] Successfully change directory to `%s`!\n", (char *)dirname);
}

// List dentry
void list_dentry(inode_t *dir_inode) {
    sbi_sd_read(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + dir_inode -> dir_ptr[0]);
    dentry_t *dentry = (dentry_t *)(pa2kva(DATABLOCK_ADDR));

    prints("%15s", "FILE NAME ");
    prints("%15s", "INODE ADDR ");
    prints("%5s", "SIZE ");
    prints("%6s", "LINKS ");
    prints("\n");
    prints("----------------------------------------\n");
    for (int i = 0; i < dir_inode -> dentry_num; i++) {
        inode_t *inode = get_inode(dentry[i].ino);
        prints("%15s", dentry[i].name);
        prints("%15x", inode);
        prints("%5d", inode -> used_size);
        prints("%5d", inode -> links);
        prints("\n");
    }
}

// List
void do_ls(uintptr_t dirname) {
    // Check if FS exists
    if (!check_fs()) return;
    
    inode_t *target_dir_inode;

    char *path = (char *)dirname;
    if (path[0] == '\0') {
        // Read dentry in current directory data block
        list_dentry(current_inode);
        return;
    }
    if ((target_dir_inode = find_path(path)) == 0) {
        prints("[ERROR] Fail to list!\n");
        return;
    }
    list_dentry(target_dir_inode);
    return;
}

/* ---------- FILE OPERATING ---------- */

// Alloc a fd
int alloc_fd() {
    for (int i = 0; i < NOFILE; i++) {
        if (FD_table[i].ino == 0)
            return i;
    }
    return -1;
}

// Create a file
// 1. Initialize inode of new file
// 2. Update data of current dir
// 3. Update inode of current dir
void do_touch(uintptr_t filename) {
    // Check if FS exists
    if (!check_fs()) return;

    inode_t *cur_dir_inode = current_inode;
    inode_t *new_file_inode = search_dir(cur_dir_inode, (char *)filename, FILE);

    if (new_file_inode != 0 && new_file_inode -> type == FILE) {
        prints("[ERROR] File has existed!\n");
        return;
    }

    // 1. Initialize inode of new file
    uint32_t ino = alloc_inode();
    new_file_inode = get_inode(ino);
    new_file_inode -> ino = ino;
    new_file_inode -> type = FILE;
    new_file_inode -> access = RW;
    new_file_inode -> links = 0;
    new_file_inode -> block_num = 1;
    new_file_inode -> used_size = 0;
    new_file_inode -> dir_ptr[0] = alloc_datablock();
    write_inode_sector(ino);

    // 2. Update data of current dir
    // Add dentry in current directory
    sbi_sd_read(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + cur_dir_inode -> dir_ptr[0]);
    dentry_t *new_dentry = (dentry_t *)(pa2kva(DATABLOCK_ADDR) + cur_dir_inode -> used_size);
    new_dentry -> type = FILE;
    new_dentry -> ino = new_file_inode -> ino;
    kstrcpy(new_dentry -> name, (char *)filename);
    sbi_sd_write(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + cur_dir_inode -> dir_ptr[0]);

    // 3. Update inode of current dir
    cur_dir_inode -> dentry_num++;
    cur_dir_inode -> used_size += sizeof(dentry_t);
    write_inode_sector(cur_dir_inode -> ino);

    prints("[FS] Successfully create file!\n");
}

// Print file content
void do_cat(uintptr_t filename) {
    // Check if FS exists
    if (!check_fs()) return;

    inode_t *target_file_inode = search_dir(current_inode, filename, FILE);
    if (target_file_inode == 0) {
        prints("[ERROR] 404 not found!\n");
        return;
    }

    char *file_data = (char *)(pa2kva(DATABLOCK_ADDR));
    char *indir1_block = (char *)(pa2kva(DATABLOCK_ADDR + SECTOR_SIZE));
    char *indir2_block = (char *)(pa2kva(DATABLOCK_ADDR + 2 * SECTOR_SIZE));
    char *indir3_block = (char *)(pa2kva(DATABLOCK_ADDR + 3 * SECTOR_SIZE));

    // We use only one sector in mem as buffer
    int cnt = 0;

    // Read from direct blocks
    for (int i = 0; i < target_file_inode -> block_num || i < MAX_SIZE_DIR; i++) {
        clear_sector(file_data);
        sbi_sd_read(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + target_file_inode -> dir_ptr[i]);
        for (int j = 0; cnt < target_file_inode -> used_size && j < SECTOR_SIZE; cnt++, j++) {
            prints("%c", file_data[j]);
        }
    }

    // Read from indir1 blocks
    for (int i = MAX_SIZE_DIR; i < target_file_inode -> block_num || i < MAX_SIZE_INDIR1; i++) {
        int indir1_block_id = (i - MAX_SIZE_DIR) / PTR_PER_SECTOR;
        int indir1_block_offset = (i - MAX_SIZE_DIR) % PTR_PER_SECTOR;

        // i == 5/133/261, read indir1 block
        if (indir1_block_offset == 0) {
            clear_sector(indir1_block);
            sbi_sd_read(DATABLOCK_ADDR + SECTOR_SIZE, 1, FS_START + DATABLOCK_OFFSET + target_file_inode -> indir1_ptr[indir1_block_id]);
        }
        // Read data blocks
        clear_sector(file_data);
        sbi_sd_read(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + indir1_block[indir1_block_offset]);
        for (int j = 0; cnt < target_file_inode -> used_size && j < SECTOR_SIZE; cnt++, j++) {
            prints("%c", file_data[j]);
        }
    }

    // Read from indir2 blocks
    for (int i = MAX_SIZE_INDIR1; i < target_file_inode -> block_num || i < MAX_SIZE_INDIR2; i++) {

        // Pay attention that this `id` has to mod 128 to get index in indir1 blocks
        // aka indir1_block_offset
        int indir2_block_id = (i - MAX_SIZE_INDIR1) / PTR_PER_SECTOR;
        int indir2_block_offset = (i - MAX_SIZE_INDIR1) % PTR_PER_SECTOR;
        int indir1_block_id = indir2_block_id / PTR_PER_SECTOR;
        int indir1_block_offset = indir2_block_id % PTR_PER_SECTOR;

        // Read indir1 block
        if (indir1_block_offset == 0) {
            clear_sector(indir1_block);
            sbi_sd_read(DATABLOCK_ADDR + SECTOR_SIZE, 1, FS_START + DATABLOCK_OFFSET + target_file_inode -> indir2_ptr[indir1_block_id]);
        }

        // Read indir2 block
        if (indir2_block_offset == 0) {
            clear_sector(indir2_block);
            sbi_sd_read(DATABLOCK_ADDR + 2 * SECTOR_SIZE, 1, FS_START + DATABLOCK_OFFSET + indir1_block[indir1_block_offset]);
        }

        // Read data block
        clear_sector(file_data);
        sbi_sd_read(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + indir2_block[indir2_block_offset]);
        for (int j = 0; cnt < target_file_inode -> used_size && j < SECTOR_SIZE; cnt++, j++) {
            prints("%c", file_data[j]);
        }
    }

    // Read from indir3 blocks
    for (int i = MAX_SIZE_INDIR2; i < target_file_inode -> block_num || i < MAX_SIZE_INDIR3; i++) {
        
        int indir3_block_id = (i - MAX_SIZE_INDIR2) / PTR_PER_SECTOR;
        int indir3_block_offset = (i - MAX_SIZE_INDIR2) % PTR_PER_SECTOR;
        int indir2_block_id = indir3_block_id / PTR_PER_SECTOR;
        int indir2_block_offset = indir3_block_id % PTR_PER_SECTOR;
        int indir1_block_id = indir2_block_id / PTR_PER_SECTOR;
        int indir1_block_offset = indir2_block_id % PTR_PER_SECTOR;

        // Read indir1 block
        if (indir1_block_offset == 0) {
            clear_sector(indir1_block);
            sbi_sd_read(DATABLOCK_ADDR + SECTOR_SIZE, 1, FS_START + DATABLOCK_OFFSET + target_file_inode -> indir3_ptr[indir1_block_id]);
        }

        // Read indir2 block
        if (indir2_block_offset == 0) {
            clear_sector(indir2_block);
            sbi_sd_read(DATABLOCK_ADDR + 2 * SECTOR_SIZE, 1, FS_START + DATABLOCK_OFFSET + indir1_block[indir1_block_offset]);
        }

        // Read indir3 block
        if (indir3_block_offset == 0) {
            clear_sector(indir3_block);
            sbi_sd_read(DATABLOCK_ADDR + 3 * SECTOR_SIZE, 1, FS_START + DATABLOCK_OFFSET + indir2_block[indir2_block_offset]);
        }

        // Read data block
        clear_sector(file_data);
        sbi_sd_read(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + indir3_block[indir3_block_offset]);
        for (int j = 0; cnt < target_file_inode -> used_size && j < SECTOR_SIZE; cnt++, j++) {
            prints("%c", file_data[j]);
        }
    }

    prints("\n");
}

int do_file_open(uintptr_t filename, int access) {

    inode_t *target_file_inode = search_dir(current_inode, (char *)filename, FILE);
    if (target_file_inode == 0) {
        prints("[ERROR] 404 not found!\n");
        return -1;
    }

    // Check access?
    // TODO:

    // Alloc a fd
    int fd = alloc_fd();
    if (fd == -1) {
        prints("[ERROR] Cannot open another file!\n");
        return -1;
    }

    FD_table[fd].ino = target_file_inode -> ino;
    FD_table[fd].access = access;
    FD_table[fd].read_point = 0;
    FD_table[fd].write_point = 0;

    return fd;
}

void do_file_close(int fd) {
    kmemset(&(FD_table[fd]), 0, sizeof(fd_t));
}


int do_file_read(int fd, uintptr_t buff, int size) {
    fd_t *file = &(FD_table[fd]);

    if (file -> ino == 0) {
        prints("[ERROR] 404 NOT FOUND\n");
        return -1;
    }
    if (file -> access == W) {
        prints("[ERROR] You do not have access to read this file!\n");
        return -1;
    }

    uint8_t *read_buff = (uint8_t *)buff;

    inode_t *target_file_inode = get_inode(file -> ino);
    uint32_t start_pos = file -> read_point;
    uint32_t end_pos = start_pos + size;
    uint32_t start_pos_offset = start_pos % SECTOR_SIZE;
    uint32_t end_pos_offset = end_pos % SECTOR_SIZE;
    uint32_t start_block_id = start_pos / SECTOR_SIZE;
    uint32_t end_block_id = (start_pos + size) / SECTOR_SIZE;

    for (int i = start_block_id; i < end_block_id; i++) {
        int start_in_this_block = i == start_block_id;
        int end_in_this_block = i == end_block_id;

        rw_datablock(target_file_inode, i, R);
        char *data_buff = pa2kva(DATABLOCK_ADDR);
        for (int j = start_in_this_block ? start_pos_offset : 0; j < end_in_this_block ? end_pos_offset : SECTOR_SIZE; j++) {
            kmemcpy(read_buff, &(data_buff[j]), 1);
            read_buff++;
        }
    }

    file -> read_point = end_pos;
    return size;
}

int do_file_write(int fd, uintptr_t buff, int size) {
    fd_t *file = &(FD_table[fd]);

    if (file -> ino == 0) {
        prints("[ERROR] 404 NOT FOUND\n");
        return -1;
    }
    if (file -> access == R) {
        prints("[ERROR] You do not have access to write this file!\n");
        return -1;
    }

    uint8_t *write_buff = (uint8_t *)buff;

    inode_t *target_file_inode = get_inode(file -> ino);
    uint32_t start_pos = file -> write_point;
    uint32_t end_pos = start_pos + size;
    uint32_t start_pos_offset = start_pos % SECTOR_SIZE;
    uint32_t end_pos_offset = end_pos % SECTOR_SIZE;
    uint32_t start_block_id = start_pos / SECTOR_SIZE;
    uint32_t end_block_id = (start_pos + size) / SECTOR_SIZE;

    // Allocate missing datablocks
    for (int i = target_file_inode -> block_num; i < start_block_id; i++) {
        set_file_block(target_file_inode, i, alloc_datablock());
        target_file_inode -> block_num ++;
    }
    for (int i = start_block_id; i < end_block_id; i++) {
        // Alloc a new datablock
        if (i == target_file_inode -> block_num) {
            set_file_block(target_file_inode, i, alloc_datablock());
            target_file_inode -> block_num++;
        }
        write_inode_sector(target_file_inode -> ino);

        int start_in_this_block = i == start_block_id;
        int end_in_this_block = i == end_block_id;

        rw_datablock(target_file_inode, i, R);
        char *data_buff = pa2kva(DATABLOCK_ADDR);
        
        for (int j = start_in_this_block ? start_pos_offset : 0; j < end_in_this_block ? end_pos_offset : SECTOR_SIZE; j++) {
            kmemcpy(&(data_buff[j]), write_buff, 1);
            write_buff++;
        }

        rw_datablock(target_file_inode, i, W);
    }

    file -> write_point = end_pos;

    target_file_inode -> used_size += size;
    write_inode_sector(target_file_inode -> ino);

    return size;
}

// Link
void do_ln(uintptr_t source, uintptr_t link_name) {
    inode_t *cur_dir_inode = current_inode;

    inode_t *target_file_inode = search_dir(cur_dir_inode, (char *)source, FILE);
    if (target_file_inode == 0) {
        prints("[ERROR] 404 source file not found!\n");
        return;
    }

    inode_t *new_file_inode = search_dir(cur_dir_inode, (char *)link_name, FILE);
    if (new_file_inode != 0 && new_file_inode -> type == FILE) {
        prints("[ERROR] File has existed!\n");
        return;
    }

    // Add dentry in current directory
    sbi_sd_read(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + cur_dir_inode -> dir_ptr[0]);
    dentry_t *new_dentry = (dentry_t *)(pa2kva(DATABLOCK_ADDR) + cur_dir_inode -> used_size);
    new_dentry -> type = FILE;
    new_dentry -> ino = target_file_inode -> ino;
    kstrcpy(new_dentry -> name, (char *)link_name);
    sbi_sd_write(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + cur_dir_inode -> dir_ptr[0]);

    // Update inode of current dir
    cur_dir_inode -> dentry_num++;
    cur_dir_inode -> used_size += sizeof(dentry_t);
    write_inode_sector(cur_dir_inode -> ino);

    target_file_inode -> links++;
    write_inode_sector(target_file_inode -> ino);
}

void do_rm(uintptr_t filename) {
    // Check if FS exists
    if (!check_fs()) return;

    inode_t *rm_file_inode = search_dir(current_inode, filename, FILE);
    if (rm_file_inode == 0) {
        prints("[ERROR] 404 not found!\n");
        return;
    }

    inode_t *cur_dir_inode = current_inode;

    // 1. Update data of current dir
    sbi_sd_read(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + cur_dir_inode -> dir_ptr[0]);
    dentry_t *dentry = (dentry_t *)(pa2kva(DATABLOCK_ADDR));
    for (int i = 0; i < cur_dir_inode -> dentry_num; i++) {
        if (dentry[i].ino == rm_file_inode -> ino) {
            if (i != cur_dir_inode -> dentry_num - 1) {
                // Move dentry[i + 1] ~ dentry[dentry_num - 1] forward
                kmemcpy(&(dentry[i]), &(dentry[i + 1]), (cur_dir_inode -> dentry_num - 1 - i) * sizeof(dentry_t));
            } else {
                kmemset(&(dentry[i]), 0, sizeof(dentry_t));
            }
            break;
        }
    }
    sbi_sd_write(DATABLOCK_ADDR, 1, FS_START + DATABLOCK_OFFSET + cur_dir_inode -> dir_ptr[0]);

    // 2. Update inode of current directory
    cur_dir_inode -> dentry_num--;
    cur_dir_inode -> used_size -= sizeof(dentry_t);
    write_inode_sector(cur_dir_inode -> ino);

    // 3. Update inode of removed file
    rm_file_inode -> links --;
    write_inode_sector(rm_file_inode -> ino);

    // Clear fd
    for (int i = 0; i < NOFILE; i++) {
        if (FD_table[i].ino == rm_file_inode) {
            kmemset(&(FD_table[i]), 0, sizeof(fd_t));
            break;
        }
    }

    if (rm_file_inode -> links == 0) {
        // 4. Free data of removed dir & Clear data bitmap
        for (int i = 0; i < rm_file_inode -> block_num; i++) {
            rw_datablock(rm_file_inode, i, FREE);
        }
        // 5. Free inode of removed file & Clear inode bitmap
        clear_imap(rm_file_inode -> ino);
        clear_inode(rm_file_inode -> ino);
    }
}

void do_lseek(int fd, int offset, int whence) {
    fd_t *file = &(FD_table[fd]);

    if (file -> ino == 0) {
        prints("[ERROR] 404 NOT FOUND\n");
        return;
    }
    
    if (whence == SEEK_SET) {
        file -> write_point = offset;
        file -> read_point = offset;
    }
    else if (whence == SEEK_CUR) {
        file -> write_point += offset;
        file -> read_point += offset;
    }

}


