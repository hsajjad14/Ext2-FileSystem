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


    inode_bitmap = (unsigned char *) (disk + 1024*bg->bg_inode_bitmap);
    int nodenum = findNextAvailableInode(sb, inode_bitmap);
    addDirectory(inode, nodenum, filename, strlen(filename), sb, bg);
    setBitmapPositionToBeOn(sb, inode_bitmap, nodenum);

    struct ext2_inode *newinode = (struct ext2_inode *) (disk + 1024*bg->bg_inode_table + 128*(nodenum-1));
    newinode->i_mode = newinode->i_mode & 0;
    newinode->i_mode = newinode->i_mode | EXT2_S_IFDIR;
    newinode->i_size = 1;
    addDirectory(newinode, nodenum, ".", 1, sb, bg);
    addDirectory(newinode, inodeIndex+1, "..", 2, sb, bg);

    return 0;
}
