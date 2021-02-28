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

int32_t ext2_fsal_mkdir(const char *path)
{
    /**
     * TODO: implement the ext2_mkdir command here ...
     * the argument path is the path to the directory to be created.
     */

     /* This is just to avoid compilation warnings, remove this line when you're done. */
    //(void)path;
    printf("--STARTED MKDIR--\n");
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024*2);
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);



    int length = strlen(path);

    char *newpath = malloc(EXT2_NAME_LEN);
    char *filename = malloc(EXT2_NAME_LEN);
    int lengthbackwards = findNameInPath((char *)path, length, filename);

    strncpy(newpath, path, lengthbackwards);
    newpath[lengthbackwards] = '\0';

    int inodeIndex = 1;
    if (strlen(newpath) > 0){
        inodeIndex = traverse(newpath, bg);
    }
    // gets the inode where to create the directory

    // make dir here in the new dir spot
    if(inodeIndex == -1){
        return ENOENT;
    }
    unsigned char *inode_bitmap = (unsigned char *) (disk + 1024*bg->bg_inode_bitmap);
    unsigned char *inodeLocation =  disk + 1024*bg->bg_inode_table + 128*inodeIndex;

    struct ext2_inode *inode = (struct ext2_inode *) (disk + 1024*bg->bg_inode_table + inodeIndex*128);
    char t = findtypeofFileOrDir(*inode, "any");
    if(t != 'd'){
        return ENOENT;
    }

    setBitmapPositionToBeOn(sb, inode_bitmap, inodeIndex);

    int check = findInode(inodeLocation, filename);
    if(check != -1){
        return EEXIST;
    }

    inode->i_links_count++; // number of dirs under this inode??

    inode_bitmap = (unsigned char *) (disk + 1024*bg->bg_inode_bitmap);
    int nodenum = findNextAvailableInode(sb, inode_bitmap);
    addDirectory(inode, nodenum, filename, strlen(filename), sb, bg);
    setBitmapPositionToBeOn(sb, inode_bitmap, nodenum);

    struct ext2_inode *newinode = (struct ext2_inode *) (disk + 1024*bg->bg_inode_table + 128*(nodenum-1));
    newinode->i_mode = newinode->i_mode & 0;
    newinode->i_mode = newinode->i_mode | EXT2_S_IFDIR;
    newinode->i_size = 1;
    newinode->i_blocks = 1;
    newinode->i_links_count = 2; // dir always has atleast 2 links
    addDirectory(newinode, nodenum, ".", 1, sb, bg);
    addDirectory(newinode, inodeIndex+1, "..", 2, sb, bg);

    return 0;
}




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

    char *pathredfnd = malloc(sizeof(dst));

    strncpy(pathredfnd, dst, strlen(dst));
    printf("len of dst: %d\n", strlen(dst));
    pathredfnd[strlen(dst)] = '\0';
    printf("dst : %s, pathredfnd: %s\n",dst, pathredfnd);
    int inodeIndex = traverse(pathredfnd, bg);
    printf("--inodeindex-- %d\n", inodeIndex);
    if(inodeIndex == -1){
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

        addFile(inode, inodeIndex, (char *)src, strlen(src), sb, bg);

        setBitmapPositionToBeOn(sb, inode_bitmap, inodeIndex);
        inodeIndex = inodeIndex - 1;
        printf("add in new dir, create inode\n");
        inode = (struct ext2_inode *) (disk + EXT2_BLOCK_SIZE*bg->bg_inode_table + inodeIndex*128);
        inode->i_links_count = 1;
        inode->i_size = sbstat.st_size;
        inode->i_mode = inode->i_mode & 0;
        inode->i_mode = inode->i_mode | EXT2_FT_REG_FILE;
        inode->i_blocks = 1;
        bg->bg_free_inodes_count--;

        printf("my printing:: inode #%d, size: %d, blocks: %d, dtime: %d\n",inodeIndex+1, inode->i_size, inode->i_blocks, inode->i_dtime);

    }
    inode->i_size = sbstat.st_size;
    inode->i_uid = 0;
    inode->osd1 = 0;
    inode->i_gid = 0;
    struct ext2_dir_entry * directory;
    int j = 0;
    inode->i_blocks = 1;
    unsigned char *f;
    unsigned char *block_bitmap = (unsigned char *) (disk + EXT2_BLOCK_SIZE*bg->bg_block_bitmap);
    int tot =  sbstat.st_size;

    // empty block
    char *empty = malloc(EXT2_BLOCK_SIZE);
    for(int x = 0; x < EXT2_BLOCK_SIZE; x++){
        empty[x] = ' ';
    }

    while(j < 12){
        if(tot > 0){
            if(inode->i_block[j] == 0){
                // find empty block
                inode->i_block[j] = findNextAvailableBlock(sb, block_bitmap);
                setBitmapPositionToBeOn(sb, block_bitmap, inode->i_block[j]);
                bg->bg_free_blocks_count--;
            }
            directory = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE*inode->i_block[j]);

            printf("before moving Inode: %d rec_len: %d name_len: %d type= u name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);


            printf("    DIR BLOCK NUM: %d (for inode %d)\n", inode->i_block[j], 12);
            f = (unsigned char *) (disk + EXT2_BLOCK_SIZE*inode->i_block[j]);
            printf("msg in current block: %s\n", f);

            memmove(directory, file + EXT2_BLOCK_SIZE*j, EXT2_BLOCK_SIZE);
            // strncpy(directory->name, file , sbstat.st_size);
            // if(tot >= EXT2_BLOCK_SIZE){
            //     memmove(directory, file + EXT2_BLOCK_SIZE*j, EXT2_BLOCK_SIZE);
            // }else{
            //     memmove(directory, file + EXT2_BLOCK_SIZE*j, tot);
            //     directory = directory+tot+1;
            //     memmove(directory, empty,10);
            //     //free(emptytemp);
            // }
            printf("msg in current block: %s\n", f);
            printf("after moving Inode: %d rec_len: %d name_len: %d type= u name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);

            //directory->name_len = sbstat.st_size;

            inode->i_blocks++;

            tot = tot - EXT2_BLOCK_SIZE;
        }else{
            if(inode->i_block[j] != 0){
                // free this block
                f = (unsigned char *) (disk + EXT2_BLOCK_SIZE*inode->i_block[j]);
                memmove(disk + EXT2_BLOCK_SIZE*inode->i_block[j], empty, EXT2_BLOCK_SIZE);
                setBitmapPositionToBeOff(sb, block_bitmap, inode->i_block[j]);
                printf("msg in current block: %s\n", f);

            }
            inode->i_block[j] = 0;

        }


        printf("j: %d\n",j );
        printf("inode block num: %d\n", inode->i_block[j]);
        printf("tot %d\n", tot);
        j++;
    }
    // j == 12, first level of indirection
    if(j*1024 <= sbstat.st_size){
        directory = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE*inode->i_block[j]);
        //...
        j++;
    }else{
        if(inode->i_block[j] != 0){
            // free this block

            memmove(disk + EXT2_BLOCK_SIZE*inode->i_block[j], empty, EXT2_BLOCK_SIZE);
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
            memmove(disk + EXT2_BLOCK_SIZE*inode->i_block[j], empty, EXT2_BLOCK_SIZE);
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
            memmove(disk + EXT2_BLOCK_SIZE*inode->i_block[j], empty, EXT2_BLOCK_SIZE);
            setBitmapPositionToBeOff(sb, block_bitmap, inode->i_block[j]);

        }
        inode->i_block[j] = 0;
        j++;
    }
    free(empty);
    printf("msg in current block: %s\n", f);
    printf("my printing:: inode #%d, size: %d, blocks: %d, dtime: %d\n",inodeIndex+1, inode->i_size, inode->i_blocks, inode->i_dtime);

    return 0;
}
