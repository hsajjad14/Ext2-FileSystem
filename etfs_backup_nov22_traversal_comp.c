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

  /**
    * searches root directory for name
    * returns:
    * 0 if found and name is a Directory
    * 1 if found and name is NOT a Directory
    * 2 if not found
    */

  int findDirectoryInInodes(char *name) {
    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024*2);
    struct ext2_inode *inode = (struct ext2_inode *) (disk + 1024*bg->bg_inode_table + 128); //root directory in inode 2
    for (int j = 14; j >= 0; j--){
      if(inode->i_block[j] != 0){
          struct ext2_dir_entry * item;
          unsigned short length = 0;
          while (length < EXT2_BLOCK_SIZE) {
            item = (struct ext2_dir_entry *) (disk + 1024*inode->i_block[j] + length);
            if (item->name == name && item->inode & EXT2_FT_DIR){
              return 0;
            } else if (item->name == name && !(item->inode & EXT2_FT_DIR){
              return 1;
            }
            length += item->rec_len;
          }
      }
    }
    return 2;
  }


  /* given a inode, find the directory location where
  * name is, return that inode
  * return location if found, -1 otherwise
  */
  int findInode(unsigned char *inodeLocation, char dirname[EXT2_NAME_LEN]){
      printf("Inside find inode\n");
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
               printf("   NAME = %s\n", directory->name);
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
     // printf("%s\n---------------\n", filename);
     int newinodenum = 0;
     while(i < lengthbackwards){
        //  printf("i: %d, A\n", i);
          nextpos = GetFirstDirName(path);

          strncpy(dirname, path+1, nextpos-1);
          dirname[nextpos-1] = '\0';
          // printf("looking for : %s\n", dirname);
          // printf("i: %d, B\n", i);
          inode = (struct ext2_inode *)  locatedir;

          type = findtypeofFileOrDir(*inode, dirname);
          if(type != 'd'){
        //      printf("notdir");
              return -1;
          }

          path = path + nextpos;

          newinodenum = findInode(locatedir, dirname);
          if(newinodenum == -1){
              return -1;
          }
          locatedir = disk + 1024*bg->bg_inode_table +128*newinodenum;


          if(nextpos == length){
             break;
          }
          i = i + nextpos;


      }
      free(filename);
      printf("inode @ %d\n", newinodenum);
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
  */
 int findNameInPath(char *path, int length, char *filname){
      int lengthbackwards = length-1;
      int numbackslashes = 0;
      if (path[lengthbackwards] == '/'){
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
      strncpy(filename, temp, length-lengthbackwards-1-numbackslashes);
      filename[length-lengthbackwards-1-numbackslashes] = '\0';

      return lengthbackwards;

  }


  /* find the 'name' under directory
   * returns:
   * 0 if found and name is a Directory
   * 1 if found and name is NOT a Directory
   * 2 if not found
   */
  int find(struct ext2_dir_entry* directory, char *name) {
    struct ext2_dir_entry * item;
    unsigned short length = 0;
    while (length < EXT2_BLOCK_SIZE) {
      item =  ((char*) directory) + length;
      if (item->name == name && item->inode & EXT2_FT_DIR){
        return 0;
      } else if (item->name == name && !(item->inode & EXT2_FT_DIR){
        return 1;
      }
      length += item->rec_len;
    }
    return 2;
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
