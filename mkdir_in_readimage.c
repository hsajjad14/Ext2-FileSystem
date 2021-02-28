#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "ext2.h"
#include <string.h>

unsigned char *disk;

int findOffset(int num) {
  if (num % 4 == 0){
    return 0;
  }
  return findOffset(num+1) + 1;
}


char *IntToBin(int n){
    char *binary = malloc(sizeof(char) * 8);
    int temp1 = 1;
    int count = 0;

    while (n > 0 || count < 8) {
        if(n > 0){
            temp1 = n%2;
            if(temp1 == 1){
                binary[count] = '1';
            }
            n = n/2;
        }else{
            binary[count] = '0';
        }

        count++;
    }
    return binary;
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
    printf("%s\n---------------\n", filename);
   int newinodenum = 0;
   while(i < length - findtrailingdashes(path)){

        printf("i: %d, A\n", i);
        nextpos = GetFirstDirName(path);

        strncpy(dirname, path+1, nextpos-1);
        dirname[nextpos-1] = '\0';
        printf("looking for : %s\n", dirname);
        inode = (struct ext2_inode *)  locatedir;

        type = findtypeofFileOrDir(*inode, dirname);
        printf("type: %c\n", type);
        if(type != 'd'){
            printf("notdir");
        }
        printf("i: %d, B\n", i);
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

int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <image file name>\n", argv[0]);
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    disk = mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if(disk == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    struct ext2_super_block *sb = (struct ext2_super_block *)(disk + 1024);
    printf("Inodes: %d\n", sb->s_inodes_count);
    printf("Blocks: %d\n", sb->s_blocks_count);
    struct ext2_group_desc *bg = (struct ext2_group_desc *)(disk + 1024*2);
    printf("Block group:\n");
    printf("    block bitmap: %d\n", bg->bg_block_bitmap);
    printf("    inode bitmap: %d\n", bg->bg_inode_bitmap);
    printf("    inode table: %d\n", bg->bg_inode_table);
    printf("    free blocks: %d\n", bg->bg_free_blocks_count);
    printf("    free inodes: %d\n", bg->bg_free_inodes_count);
    printf("    used_dirs: %d\n", bg->bg_used_dirs_count);

    unsigned char * block_bitmap = (unsigned char *) (disk + 1024*bg->bg_block_bitmap);
    printf("Block bitmap: ");
    for (int byte = 0; byte < sb->s_blocks_count/8; byte++) {
      int stringForm = (int)block_bitmap[byte];
      char *binary = IntToBin(stringForm);
      printf("%s ", binary);
    }
    unsigned char * inode_bitmap = (unsigned char *) (disk + 1024*bg->bg_inode_bitmap);
    printf("\n");
    printf("Inode bitmap: ");
    for (int byte = 0; byte < sb->s_inodes_count/8; byte++) {
      int stringForm = (int)inode_bitmap[byte];
      char *binary = IntToBin(stringForm);
      printf("%s ", binary);
    }

    printf("\n\nInodes:\n");
    struct ext2_inode *inode;

    for(int i = 0; i < sb->s_inodes_count; i++){
        inode = (struct ext2_inode *) (disk + 1024*bg->bg_inode_table + i*128); //128 = size of inode
        if(i == EXT2_ROOT_INO-1 || (i > EXT2_GOOD_OLD_FIRST_INO-1 && inode->i_size > 0)){
            if(EXT2_S_IFREG & inode->i_mode){
                printf("[%d] type: f size: %d links: %d blocks: %d\n",i+1 , inode->i_size,inode->i_links_count, inode->i_blocks);
            }
            else if(EXT2_S_IFLNK & inode->i_mode){
                printf("[%d] type: l size: %d links: %d blocks: %d\n",i+1 , inode->i_size,inode->i_links_count, inode->i_blocks);
            }
            else if(EXT2_S_IFDIR & inode->i_mode){
                printf("[%d] type: d size: %d links: %d blocks: %d\n",i+1 , inode->i_size,inode->i_links_count, inode->i_blocks);
            }
            printf("[%d] Blocks: ", i+1);
            for(int j = 14; j >= 0; j--){
                if(inode->i_block[j] != 0){
                    printf("%d", inode->i_block[j]);
                }

            }
            printf("\n");
        }

    }

    printf("\nDirectory Blocks:\n");

    for(int i = 0; i < sb->s_inodes_count; i++) {
      inode = (struct ext2_inode *) (disk + 1024*bg->bg_inode_table + i*128); //128 = size of inode

      if (i == EXT2_ROOT_INO-1 || (i > EXT2_GOOD_OLD_FIRST_INO-1 && inode->i_size > 0)) {
        if (EXT2_S_IFDIR & inode->i_mode) {
          for(int j = 14; j >= 0; j--){
              if(inode->i_block[j] != 0){
                  printf("    DIR BLOCK NUM: %d (for inode %d)\n", inode->i_block[j], i+1);
                  struct ext2_dir_entry * directory;
                  unsigned short length = 0;
                  while (length < EXT2_BLOCK_SIZE) {
                    directory = (struct ext2_dir_entry *) (disk + 1024*inode->i_block[j] + length);
                    if(directory->file_type & EXT2_FT_UNKNOWN){
                        printf("Inode: %d rec_len: %d name_len: %d type= u name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);
                    }
                    else if(directory->file_type & EXT2_FT_REG_FILE){
                        printf("Inode: %d rec_len: %d name_len: %d type= f name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);
                    }
                    else if(directory->file_type & EXT2_FT_DIR){
                        printf("Inode: %d rec_len: %d name_len: %d type= d name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);
                    }
                    else if(directory->file_type & EXT2_FT_SYMLINK){
                        printf("Inode: %d rec_len: %d name_len: %d type= l name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);
                    }
                    length += directory->rec_len;
                  }
              }
          }
        }
      }
    }



    printf("------------------\n");
    printf("for path: \"/level1/testingHaider\" traverse inodes and print the file/dir names \n");


    char *path = "/level1/testingHaider//////////";
    int length = strlen(path);

    char *newpath = malloc(EXT2_NAME_LEN);
    char *filename = malloc(EXT2_NAME_LEN);
    int lengthbackwards = findNameInPath(path, length, filename);
    printf("length: %d, lengthbackwards: %d\n", length, lengthbackwards);
    printf("-------------\n");
    strncpy(newpath, path, lengthbackwards);
    printf("length = %d, lengthback = %d, findtrailingdashes %d\n",length, lengthbackwards, findtrailingdashes(newpath));
    newpath[lengthbackwards] = '\0';
    printf("file is %s\n", filename);
    printf("newpath = %s, old path = %s\n", newpath, path);
    printf("-----------------\n");
    int inodeIndex = traverse(newpath, bg);
    printf("after traverse, inodeIndex = %d\n",inodeIndex );
    printf("-----------------\n");
    inode = (struct ext2_inode *) (disk + 1024*bg->bg_inode_table + inodeIndex*128);
    // for(int j = 14; j >= 0; j--){
    //     if(inode->i_block[j] != 0){
    //         //printf("    DIR BLOCK NUM: %d (for inode %d)\n", inode->i_block[j], i+1);
    //         struct ext2_dir_entry * directory;
    //         unsigned short length = 0;
    //         while (length < EXT2_BLOCK_SIZE) {
    //           directory = (struct ext2_dir_entry *) (disk + 1024*inode->i_block[j] + length);
    //           if(directory->file_type & EXT2_FT_UNKNOWN){
    //               printf("Inode: %d rec_len: %d name_len: %d type= u name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);
    //           }
    //           else if(directory->file_type & EXT2_FT_REG_FILE){
    //               printf("Inode: %d rec_len: %d name_len: %d type= f name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);
    //           }
    //           else if(directory->file_type & EXT2_FT_DIR){
    //               printf("Inode: %d rec_len: %d name_len: %d type= d name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);
    //           }
    //           else if(directory->file_type & EXT2_FT_SYMLINK){
    //               printf("Inode: %d rec_len: %d name_len: %d type= l name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);
    //           }
    //           length += directory->rec_len;
    //         }
    //     }
    // }

//     int a = 0;
//     while(a < 15){
//         if(inode->i_block[a] == 0 && a < 13){
//             break;
//         }
//         a++;
//     }
//     printf("inode index: %d\n",inodeIndex+1);
// //    printf("SEG\n");
//     block_bitmap = (unsigned char *) (disk + 1024*bg->bg_block_bitmap);
// //    printf("C\n");
//     int nextblock = findNextAvailableBlock(sb, block_bitmap);
//     setBitmapPositionToBeOn(sb, block_bitmap, nextblock);
//     // update block bitmap
// //    printf("A\n");
//     printf("a: %d block picked: %d\n", a, nextblock);
//     inode->i_block[a] = nextblock;
//     printf("directory used by %s, %d\n",filename, nextblock);
    inode_bitmap = (unsigned char *) (disk + 1024*bg->bg_inode_bitmap);
    int nodenum = findNextAvailableInode(sb, inode_bitmap);
    addDirectory(inode, nodenum, filename, strlen(filename), sb, bg);
    setBitmapPositionToBeOn(sb, inode_bitmap, nodenum);
    printf("node picked %d\n", nodenum);
//     printf("nodenum1 :%d\n",nodenum );
    struct ext2_inode *newinode = (struct ext2_inode *) (disk + 1024*bg->bg_inode_table + 128*(nodenum-1));
    newinode->i_mode = newinode->i_mode | EXT2_S_IFDIR;
    newinode->i_size = 1;
    printf("node again: %d\n",nodenum);
    addDirectory(newinode, nodenum, ".", 1, sb, bg);
    addDirectory(newinode, inodeIndex +1, "..", 2, sb, bg);

// // //     printf("inode bitmap %u\n", inode_bitmap);
// // // //    printf("D\n");
//     struct ext2_dir_entry * directory = (struct ext2_dir_entry *) (disk + 1024*nextblock);
// // //
// // // //    printf("E\n");
//     directory->name_len = strlen(filename);
// // // //    printf("H\n");
//     directory->file_type = directory->file_type | EXT2_FT_DIR;
//     directory->inode = nodenum;
// // //
//     strncpy(directory->name,filename,directory->name_len);
//     directory->rec_len = 1024;
//     nextblock = findNextAvailableBlock(sb, block_bitmap);
    // update block bitmap
//     setBitmapPositionToBeOn(sb, block_bitmap, nextblock);
//     int check = findNextAvailableBlock(sb, block_bitmap);
//     printf("new block 2 %d, nextblock? %d", nextblock, check);
//     setBitmapPositionToBeOn(sb, block_bitmap, check);
//     check = findNextAvailableBlock(sb, block_bitmap);
//     printf(", nextblock? %d", check);
//     setBitmapPositionToBeOn(sb, block_bitmap, check);
//     check = findNextAvailableBlock(sb, block_bitmap);
//     printf(", nextblock? %d", check);
//     setBitmapPositionToBeOn(sb, block_bitmap, check);
//     check = findNextAvailableBlock(sb, block_bitmap);
//     printf(", nextblock? %d", check);
//     setBitmapPositionToBeOn(sb, block_bitmap, check);
//     check = findNextAvailableBlock(sb, block_bitmap);
//     printf(", nextblock? %d \n", check);
//
//     printf("dir node %d = 13\n",directory->inode );
//     newinode = (struct ext2_inode *) (disk + 1024*bg->bg_inode_table + (directory->inode-1)*128);
//     a = 0;
//     while(a < 15){
//         if(newinode->i_block[a] == 0 && a < 13){
//             break;
//         }
//         a++;
//     }
//     newinode->i_block[a] = nextblock;
//     newinode->i_mode = newinode->i_mode | EXT2_S_IFDIR;
//     newinode->i_size = 1;
//     directory = (struct ext2_dir_entry *) (disk + 1024*nextblock);
//     directory->name_len = 1;
//     directory->file_type = directory->file_type | EXT2_FT_DIR;
//     directory->inode = nodenum;
//     strncpy(directory->name,".", 1);
//     directory->rec_len = 80;
// //
//
//     directory = (struct ext2_dir_entry *) (disk + 1024*nextblock + 80);
//     directory->name_len = 2;
//     directory->file_type = directory->file_type | EXT2_FT_DIR;
//     directory->inode = inodeIndex + 1;
//     strncpy(directory->name,"..", 2);
//     directory->rec_len = 1024;
//
//
// //
    printf("------------\n");
    for(int i = 0; i < sb->s_inodes_count; i++) {
      inode = (struct ext2_inode *) (disk + 1024*bg->bg_inode_table + i*128); //128 = size of inode
    // printf("inode: %d, size: %d, mode: %d\n", i+1, inode->i_size,  inode->i_mode);
      if (i == EXT2_ROOT_INO-1 || (i > EXT2_GOOD_OLD_FIRST_INO-1 && inode->i_size > 0)) {

        if (EXT2_S_IFDIR & inode->i_mode) {
    //        printf("made it in\n");
          for(int j = 14; j >= 0; j--){
              // if(i+1 == 13){
              //     printf("%d\n", inode->i_block[j]);
              // }
              if(inode->i_block[j] != 0){
                  printf("    DIR BLOCK NUM: %d (for inode %d)\n", inode->i_block[j], i+1);
                  struct ext2_dir_entry * directory;
                  unsigned short length = 0;
                  while (length < EXT2_BLOCK_SIZE) {
                    directory = (struct ext2_dir_entry *) (disk + 1024*inode->i_block[j] + length);
                    if(directory->file_type & EXT2_FT_UNKNOWN){
                        printf("Inode: %d rec_len: %d name_len: %d type= u name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);
                    }
                    else if(directory->file_type & EXT2_FT_REG_FILE){
                        printf("Inode: %d rec_len: %d name_len: %d type= f name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);
                    }
                    else if(directory->file_type & EXT2_FT_DIR){
                        printf("Inode: %d rec_len: %d name_len: %d type= d name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);
                    }
                    else if(directory->file_type & EXT2_FT_SYMLINK){
                        printf("Inode: %d rec_len: %d name_len: %d type= l name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);
                    }
                    length += directory->rec_len;
                  }
              }
          }
        }
      }
    }





    return 0;
}
