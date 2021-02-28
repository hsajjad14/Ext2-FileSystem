/*
 *------------
 * This code is provided solely for the personal and private use of
 * students taking the CSC369H5 course at the University of Toronto.
 * Copying for purposes other than this use is expressly prohibited.
 * All forms of distribution of this code, whether as given or with
 * any changes, are expressly prohibited.
 *
 * All of the files in this directory and all subdirectories are:
 * Copyright (c) 2019 MCS @ UTM
 * -------------
 */

#include "ext2fsal.h"
#include "e2fs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


int32_t ext2_fsal_cp(const char *src,
                     const char *dst)
{
    /**
     * TODO: implement the ext2_cp command here ...
     * src and dst are the ln command arguments described in the handout.
     */

     /* This is just to avoid compilation warnings, remove these 2 lines when you're done. */
    (void)src;
    (void)dst;
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024*2);

    int fd = open(src, O_RDWR);
    printf("fd %d\n", fd);

    struct stat sbstat;
    fstat(fd, &sbstat);

    char *file = mmap(NULL, sbstat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    char *filename = malloc(EXT2_NAME_LEN);
    int length = strlen(src);
    findNameInPath((char *)src, length, filename);

    char *pathredfnd = malloc(sizeof(dst));

    strncpy(pathredfnd, dst, strlen(dst));
    printf("len of dst: %d\n", strlen(dst));
    pathredfnd[strlen(dst)] = '\0';
    printf("dst : %s, pathredfnd: %s\n",dst, pathredfnd);
    int inodeIndex = traverse(pathredfnd, bg);
    printf("--inodeindex-- %d\n", inodeIndex);
    if(inodeIndex == -1){
        munmap(file, sbstat.st_size);
        close(fd);
        return ENOENT;
    }
    struct ext2_inode *inode = (struct ext2_inode *) (disk + EXT2_BLOCK_SIZE*bg->bg_inode_table + inodeIndex*128);

    char type = findtypeofFileOrDir(*inode, "any");
    unsigned char *inode_bitmap = (unsigned char *) (disk + 1024*bg->bg_inode_bitmap);
    printf("type--%c\n",type);
    printf("my printing:: inode #%d, size: %d, blocks: %d, dtime: %d\n",inodeIndex, inode->i_size, inode->i_blocks, inode->i_dtime);

    if(type == 'd'){
        inodeIndex = findNextAvailableInode(sb, inode_bitmap);
        printf("--inodeindex found-- %d\n", inodeIndex);

        addFile(inode, inodeIndex, filename, strlen(filename), sb, bg);
        bg->bg_free_inodes_count--;
        setBitmapPositionToBeOn(sb, inode_bitmap, inodeIndex);
        inodeIndex = inodeIndex - 1;
        printf("add in new dir, create inode\n");
        inode = (struct ext2_inode *) (disk + EXT2_BLOCK_SIZE*bg->bg_inode_table + inodeIndex*128);
        inode->i_links_count = 1;
        inode->i_size = sbstat.st_size;
        inode->i_mode = EXT2_S_IFREG;
        printf("my printing:: inode #%d, size: %d, blocks: %d, dtime: %d\n",inodeIndex+1, inode->i_size, inode->i_blocks, inode->i_dtime);
    }
    inode->i_size = sbstat.st_size;
    inode->i_uid = 0;
    inode->osd1 = 0;
    inode->i_gid = 0;
    struct ext2_dir_entry * directory;
    int j = 0;
    unsigned char *f;
    unsigned char *block_bitmap = (unsigned char *) (disk + EXT2_BLOCK_SIZE*bg->bg_block_bitmap);
    char *empty = malloc(EXT2_BLOCK_SIZE);
    for(int x = 0; x < EXT2_BLOCK_SIZE; x++){
        empty[x] = ' ';
    }
    int tot =  sbstat.st_size;
    int numblocks = 0;
    while(j < 12 && tot > 0){

        if(inode->i_block[j] == 0){
            // find empty block
            inode->i_block[j] = findNextAvailableBlock(sb, block_bitmap);
            setBitmapPositionToBeOn(sb, block_bitmap, inode->i_block[j]);


        }
        directory = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE*inode->i_block[j]);

        printf("before moving Inode: %d rec_len: %d name_len: %d type= u name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);


        printf("    DIR BLOCK NUM: %d (for inode %d)\n", inode->i_block[j], 12);
        f = (unsigned char *) (disk + EXT2_BLOCK_SIZE*inode->i_block[j]);
        printf("msg in current block: %s\n", f);
        printf("inode num ---- %d\n", inodeIndex);


        // strncpy(directory->name, file , sbstat.st_size);
        memmove(directory, file + EXT2_BLOCK_SIZE*j, EXT2_BLOCK_SIZE);
        //msync(file, EXT2_BLOCK_SIZE, MS_SYNC);
        printf("msg in current block: %s\n", f);
        printf("after moving Inode: %d rec_len: %d name_len: %d type= u name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);

        //directory->name_len = sbstat.st_size;
        numblocks++;
        //inode->i_blocks++;
        printf("j: %d\n",j );
        j++;
        tot = tot - EXT2_BLOCK_SIZE;
        printf("tot %d\n", tot);

    }
    // turn off blocks that are no longer being used
    while(j < 12){
        if(inode->i_block[j] != 0){
            setBitmapPositionToBeOff(sb, block_bitmap, inode->i_block[j]);
            inode->i_block[j] = 0;
        }
        j++;
    }
    printf("check if changes \n");
    inode->i_blocks = numblocks*2;
    // j == 12, first level of indirection
    if(j*1024 <= sbstat.st_size){
        directory = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE*inode->i_block[j]);
        //...
        j++;
    }else{
        if(inode->i_block[j] != 0){
            // free this block

        //    memmove(disk + EXT2_BLOCK_SIZE*inode->i_block[j], empty, EXT2_BLOCK_SIZE);
            setBitmapPositionToBeOff(sb, block_bitmap, inode->i_block[j]);

        }
        inode->i_block[j] = 0;
        j++;
    }
    // j == 13, first level of indirection
    if(j*1024 <= sbstat.st_size){
        directory = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE*inode->i_block[j]);
        //...
        j++;
    }else{
        if(inode->i_block[j] != 0){
            // free this block
            //memmove(disk + EXT2_BLOCK_SIZE*inode->i_block[j], empty, EXT2_BLOCK_SIZE);
            setBitmapPositionToBeOff(sb, block_bitmap, inode->i_block[j]);

        }
        inode->i_block[j] = 0;
        j++;
    }
    // j == 14, first level of indirection
    if(j*1024 <= sbstat.st_size){
        directory = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE*inode->i_block[j]);
        //...
        j++;
    }else{
        if(inode->i_block[j] != 0){
            // free this block
        //    memmove(disk + EXT2_BLOCK_SIZE*inode->i_block[j], empty, EXT2_BLOCK_SIZE);
            setBitmapPositionToBeOff(sb, block_bitmap, inode->i_block[j]);

        }
        inode->i_block[j] = 0;
        j++;
    }

    munmap(file, sbstat.st_size);
    close(fd);
    printf("my printing:: inode #%d, size: %d, blocks: %d, dtime: %d\n",inodeIndex+1, inode->i_size, inode->i_blocks, inode->i_dtime);

    return 0;
}
