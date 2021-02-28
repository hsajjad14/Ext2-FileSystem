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

int findNextAvailableInode(struct ext2_super_block *sb, unsigned char * inode_bitmap) {
  int counter = 0;
  for (int byte = 0; byte < sb->s_inodes_count/8; byte++) {
    for (int bit = 0; bit < 8; bit ++) {
      counter++;
      int in_use = inode_bitmap[byte] & (1 << bit);
      if (!in_use) {
        return counter;
      }
    }
  }
  return -1;
}

//finds the next available block from the block bitmap, returns -1 it all are used
int findNextAvailableBlock(struct ext2_super_block *sb, unsigned char * block_bitmap) {
  int counter = 0;

  for (int byte = 0; byte < sb->s_blocks_count/8; byte++) {
      int stringForm = (int)block_bitmap[byte];
      char *binary = IntToBin(stringForm);
      printf("2\n");
        // for (int bit = 0; bit < 8; bit ++) {
        //   counter++;
        //   printf("counter in fnb : %d\n", counter);
        //   int in_use = binary[bit];
        //   if (in_use == '0') {
        //     return counter;
        //   }
        // }
  }
  return -1;
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
    printf("%s\n---------------\n", filename);
   int newinodenum = 0;
   while(i < lengthbackwards){

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
    printf("for path: \"/level1/bfile\" traverse inodes and print the file/dir names \n");

    char *path = "/level1/bfile";
    int inodeIndex = traverse(path, bg);

    printf("after traverse, inodeIndex = %d\n",inodeIndex );
    inode = (struct ext2_inode *) (disk + 1024*bg->bg_inode_table + inodeIndex*128);
    for(int j = 14; j >= 0; j--){
        if(inode->i_block[j] != 0){
            //printf("    DIR BLOCK NUM: %d (for inode %d)\n", inode->i_block[j], i+1);
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





    return 0;
}
