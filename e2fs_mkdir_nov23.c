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

/**
 * TODO: Make sure to add all necessary includes here.
 */

#include "e2fs.h"
#include <string.h>
 /**
  * TODO: Add any helper implementations here.
  */

  // .....

  /* given a inode, find the directory location where
  * name is, return that inode
  * return location if found, -1 otherwise
  */
  int findInode(unsigned char *inodeLocation, char dirname[EXT2_NAME_LEN]){
     // printf("Inside find inode\n");
      struct ext2_inode  *givenInode = (struct ext2_inode  *) inodeLocation;
      int j = 0 ;
      struct ext2_dir_entry * directory;
      int newinodenum = 0;
      while(j < 15){
       //printf(" --- j: %d\n", j);
       //printf("%d\n", inode->i_block[j]);
          if(givenInode->i_block[j] != 0){
          //    printf("    DIR BLOCK NUM: %d (for inode %d)\n", inode->i_block[j], i+1);

              unsigned short length = 0;
              while (length < EXT2_BLOCK_SIZE) {
               directory = (struct ext2_dir_entry *) (disk + 1024*givenInode->i_block[j] + length);
        //       printf("   NAME = %s\n", directory->name);
                if(strncmp(directory->name, dirname, directory->name_len) == 0){
                    // inode = (struct ext2_inode *)  (disk + 1024*bg->bg_inode_table +128*inode->i_block[j]);
                    // break;
                    //return (unsigned char *) (disk + 1024*inode->i_block[j]);
                   // printf("found %s !, in dir its %.*s, inode @ inode #: %d, block inode points to is: %d\n",dirname, directory->name_len, directory->name,directory->inode, inode->i_block[j]);
                    newinodenum = directory->inode - 1;
                    return newinodenum;
                }
          //
                length += directory->rec_len;
              }
          }
          j++;

      }

      return -1;

  }
  /*
  * find number of numbackslashes at the end of the path
  */
  int findtrailingdashes(char *path){
      int length = strlen(path);
      int numbackslashes = 0;
      int back = length -1 ;
      while(path[back] == '/'){
          numbackslashes ++;
          back--;
      }
      return numbackslashes;

  }
  // returns inode holding second last file/dir in path
  // "/foo/bar/blah/csc/369/haider/" => returns inode of dir 369
  int traverse(char *path, struct ext2_group_desc *bg){
      int length = strlen(path);
      int i = 0;
      char dirname[EXT2_NAME_LEN];
      int nextpos;
      char type;
      //root inode

      unsigned char *locatedir = disk + 1024*bg->bg_inode_table + 128;// start at root

      struct ext2_inode  *inode;
      // finding the dirname
      char *filename = malloc(EXT2_NAME_LEN);

      int lengthbackwards = findNameInPath(path, length, filename);
    //  printf("%s\n---------------\n", filename);
     int newinodenum = 0;
     while(i < length - findtrailingdashes(path)){

    //      printf("i: %d, A\n", i);
          nextpos = GetFirstDirName(path);

          strncpy(dirname, path+1, nextpos-1);
          dirname[nextpos-1] = '\0';
          if(strlen(dirname) != 0){
              inode = (struct ext2_inode *)  locatedir;

              type = findtypeofFileOrDir(*inode, dirname);
        //      printf("type: %c\n", type);
              if(type != 'd'){
        //          printf("notdir");
                  return -1;
              }
        //      printf("i: %d, B\n", i);
              path = path + nextpos;

              newinodenum = findInode(locatedir, dirname);
              if(newinodenum == -1){
                  return -1;
              }
              locatedir = disk + 1024*bg->bg_inode_table +128*newinodenum;

          }else{
              path = path + nextpos;
          }


          if(nextpos == length){
             break;
          }
          i = i + nextpos;


      }
      free(filename);
    //  printf("inode @ %d\n", newinodenum);
      return newinodenum;
  }


  /*
 * given an inode and file or dir name
 * find the type if that file or dir name, (f, d, u)
 */
 char findtypeofFileOrDir(struct ext2_inode inode, char name[EXT2_NAME_LEN]){

    for(int j = 14; j >= 0; j--){
        if(inode.i_block[j] != 0){
        //    printf("    DIR BLOCK NUM: %d (for inode %d)\n", inode->i_block[j], i+1);
            struct ext2_dir_entry * directory;
            unsigned short length = 0;
            while (length < EXT2_BLOCK_SIZE) {
              directory = (struct ext2_dir_entry *) (disk + 1024*inode.i_block[j] + length);
              if(strncmp(directory->name, name, directory->name_len) == 0){
                  if(EXT2_S_IFREG & inode.i_mode){
                      return 'f';
                  }
                  else if(EXT2_S_IFLNK & inode.i_mode){
                      return 'l';
                  }
                  else if(EXT2_S_IFDIR & inode.i_mode){
                      return 'd';
                  }
              }
             // printf("name given = %s, name in dir = %.*s\n", name, directory->name_len, directory->name );

              length += directory->rec_len;
            }
        }
    }
    return 'u';

 }
  /*
  * given "/foo/bar/blah/csc/369/haider/" => change filename to  haider
  * or given "/foo/bar/blah/csc/369/haider" => change filename to  haider
  * also returns length of "/foo/bar/blah/csc/369/"
  */
  int findNameInPath(char *path, int length, char *filename){
        int lengthbackwards = length-1;
        int numbackslashes = 0;
        while (path[lengthbackwards] == '/'){
            lengthbackwards--;
            numbackslashes++;

        }

        while(1){
            if(path[lengthbackwards] == '/'){
                break;
            }
            lengthbackwards--;
        }
        char *temp = path + lengthbackwards +1;
        //printf("temp = %s, lenback = %d, length = %d\n", temp, lengthbackwards, length);

        strncpy(filename, temp, length-lengthbackwards-1-numbackslashes);
          filename[length-lengthbackwards-1-numbackslashes] = '\0';

        return lengthbackwards;

    }


  /**
  * Get the starting position of the next directory, or name in the path.
  */
  int GetFirstDirName(char *s){
      int i = 1;
      while(s[i] != '/' && i < strlen(s)){

          i += 1;
      }
      return i;
  }


//finds the next available inode from the inode bitmap, returns -1 if all are used
int findNextAvailableInode(struct ext2_super_block *sb, unsigned char * inode_bitmap) {
  int counter = 0;
  for (int byte = 0; byte < sb->s_inodes_count/8; byte++) {
    for (int bit = 0; bit < 8; bit ++) {
      counter++;
      if(counter > EXT2_GOOD_OLD_FIRST_INO){
          int in_use = inode_bitmap[byte] & (1 << bit);
          if (!in_use) {
            return counter;
          }
      }

    }
  }
  return -1;
}

//finds the next available block from the block bitmap, returns -1 it all are used
int findNextAvailableBlock(struct ext2_super_block *sb, unsigned char * block_bitmap) {
  int counter = 0;
  for (int byte = 0; byte < sb->s_blocks_count/8; byte++) {
    for (int bit = 0; bit < 8; bit ++) {
      counter++;
      int in_use = block_bitmap[byte] & (1 << bit);
      if (!in_use) {
        return counter;
      }
    }
  }
  return -1;
}

//offset to make num to next multiple of 4
int findOffset(int num) {
  if (num % 4 == 0){
    return 0;
  }
  return findOffset(num+1) + 1;
}

// sets bitmap at position n to be on
void setBitmapPositionToBeOn(struct ext2_super_block *sb, unsigned char * bitmap, int n){
    int counter = 0;
    for (int byte = 0; byte < sb->s_inodes_count/8; byte++) {
      for (int bit = 0; bit < 8; bit ++) {
        counter++;

        if (n == counter) {
          bitmap[byte] = bitmap[byte] | (1 << bit);
        }
      }
    }

}
int addDirectory(struct ext2_inode* parent, int parentInodeNum, char *name, int name_len, struct ext2_super_block* sb, struct ext2_group_desc *bg) {
  for(int j = 14; j >= 0; j--){
      if(parent->i_block[j] != 0){
          struct ext2_dir_entry * directory;
          unsigned short length = 0;
          int addedDir = 0;
          while (length < EXT2_BLOCK_SIZE && !addedDir) {
            directory = (struct ext2_dir_entry *) (disk + 1024*parent->i_block[j] + length);
            if (length + directory->rec_len == EXT2_BLOCK_SIZE) {
              directory->rec_len = 8 + directory->name_len + findOffset(8 + directory->name_len);
              struct ext2_dir_entry * newdirectory = (struct ext2_dir_entry *) (disk + 1024*parent->i_block[j] + (length + directory->rec_len));
              newdirectory->rec_len = 1024 - (length + directory->rec_len);
              strncpy(newdirectory->name, name, name_len);
              newdirectory->inode = parentInodeNum;
              newdirectory->file_type = newdirectory->file_type | EXT2_FT_DIR;
              newdirectory->name_len = name_len;
              //addedDir = 1;
              return 1;
            }
            length += directory->rec_len;
          }
      }
  }

  int a = 0;
  while(a < 15){
      if(parent->i_block[a] == 0 && a < 13){
          break;
      }
      a++;
  }
  unsigned char *block_bitmap = (unsigned char *) (disk + 1024*bg->bg_block_bitmap);
  int nextblock = findNextAvailableBlock(sb, block_bitmap);
  setBitmapPositionToBeOn(sb, block_bitmap, nextblock);
  parent->i_block[a] = nextblock;
  struct ext2_dir_entry * newdirectory = (struct ext2_dir_entry *) (disk + 1024*nextblock);
  newdirectory->rec_len = 1024;
  strncpy(newdirectory->name, name, name_len);
  newdirectory->inode = parentInodeNum;
  newdirectory->file_type = newdirectory->file_type | EXT2_FT_DIR;
  newdirectory->name_len = name_len;

  return 2;

}
