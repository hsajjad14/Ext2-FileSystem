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

    printf("------------\n");
    printf("finding all directories under indoe 12\n");
    inode = (struct ext2_inode *) (disk + 1024*bg->bg_inode_table + 11*128); //128 = size of inode, start at root
    for(int j = 14; j >= 0; j--){
        if(inode->i_block[j] != 0){
        //    printf("    DIR BLOCK NUM: %d (for inode %d)\n", inode->i_block[j], i+1);
            struct ext2_dir_entry * directory;
            unsigned short length = 0;
            while (length < EXT2_BLOCK_SIZE) {
              directory = (struct ext2_dir_entry *) (disk + 1024*inode->i_block[j] + length);
              if(strncmp(directory->name, "bfile", directory->name_len) == 0){
                  printf("found name, %.*s = bfile\n", directory->name_len, directory->name);
              }
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

    printf("------------------\n");
    printf("writing a directory to inode 12, (level1)\n");
    inode = (struct ext2_inode *) (disk + 1024*bg->bg_inode_table + 128);
    int j = 12;
    // while(j< 15){
    //     if(inode->i_block[j] == 0){
    //         break;
    //     }
    //     j++;
    // }
    // for(int i = 14; i >= 0; i--){
    //     if(inode->i_block[i] != 0){
    //         int y = (int)inode->i_block[i];
    //         printf("%d\n", y);
    //     }
    //
    // }


    printf("C\n");
    int nextblock = 35;//findNextAvailableBlock(sb, (unsigned char *) bg->bg_block_bitmap);
    printf("A\n");
    inode->i_block[j] = nextblock;
    int nodenum = 30;// findNextAvailableInode(sb, (unsigned char *) bg->bg_inode_bitmap);
    printf("B\n");
    struct ext2_inode *newinode = (struct ext2_inode *) (disk + 1024*bg->bg_inode_table + 128*nodenum);
    printf("D\n");
    struct ext2_dir_entry * directory = (struct ext2_dir_entry *) (disk + 1024*nextblock);

    printf("E\n");
    directory->name_len = 13;
    printf("H\n");
    directory->file_type = directory->file_type | EXT2_FT_DIR;
    // // inode num 16?
    directory->inode = nodenum;

    strncpy(directory->name,"testingHaider",13);
    directory->rec_len = 1024;
    // go to inode 16.
    // in its data blocks make directories
    struct ext2_inode *newinode = (struct ext2_inode *) (disk + 1024*bg->bg_inode_table + directory->inode*128);
    nextblock = 101;//findNextAvailableBlock(sb, (unsigned char *) bg->bg_block_bitmap);
    directory = (struct ext2_dir_entry *) (disk + 1024*nextblock);
    printf("F\n");

    // directory = (struct ext2_dir_entry *) (disk + 1024*newinode->i_block[0]);
    directory->name_len = 1;
    directory->file_type = directory->file_type | EXT2_FT_DIR;
    // // inode num 16?
    // directory->inode = nodenum;
    // strncpy(directory->name,".",1);
    // directory->rec_len = 1024;
    // nextblock = 102;//findNextAvailableBlock(sb, (unsigned char *) bg->bg_block_bitmap);

    //
    // directory = (struct ext2_dir_entry *) (disk + 1024*nextblock);
    // // directory = (struct ext2_dir_entry *) (disk + 1024*newinode->i_block[0]+ 12);
    // directory->name_len = 2;
    // directory->file_type = directory->file_type | EXT2_FT_DIR;
    // // // inode num 16?
    // directory->inode = 2;
    // strncpy(directory->name,"..",2);
    // directory->rec_len = 32;


    return 0;
}
