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
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024*2);

    int inodeIndex = traverse(path, bg);
    // gets the inode where to create the directory

    // make dir here in the new dir spot
    if(inodeIndex == -1){
        return ENOENT;
    }
    unsigned char *inodeLocation =  disk + 1024*bg->bg_inode_table + 128*inodeIndex;

    char *filename = malloc(EXT2_NAME_LEN);
    findNameInPath(path, length, filename);
    int check = findInode(inodeLocation, filename)
    if(check != -1){
        return EEXIST;
    }

    int nextblock = ... find next free block;
    int nextInode = ... find next free inode;
    int j = 0;
    while(j< 15){
        if(inode->i_block[j] == 0){
            break;
        }
        j++;
    }
    inode->i_block[j] = nextblock;
    struct ext2_inode  *inode = (struct ext2_inode *) (disk + 1024*bg->bg_inode_table + 128*inodeIndex);

    struct ext2_dir_entry * directory = (struct ext2_dir_entry *) (disk + 1024*nextblock);
    int length = strlen(filename);
    directory->name_len = length;
    directory->file_type = directory->file_type | EXT2_FT_DIR;
    directory->inode = nextInode;
    strncpy(directory->name, filename, directory->name_len);
    directory->rec_len = ...?;
    // adding directories . and .. to our new file
    inode = (struct ext2_inode *)  (disk + 1024*bg->bg_inode_table + directory->inode*128);
    int nextblock = ... find next free block; // for "."
    j = 0;
    while(j< 15){
        if(inode->i_block[j] == 0){
            break;
        }
        j++;
    }
    inode->i_block[j] = nextblock;
    directory = (struct ext2_dir_entry *) (disk + 1024*nextblock); // is this right?
    directory->name_len = 1;
    directory->file_type = directory->file_type | EXT2_FT_DIR;
    directory->inode = nextInode; // point it back o same inode as parent

    // adding directory for ".."
    ... same thing as above






    return 0;
}
