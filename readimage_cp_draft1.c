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
    int i = 0;
    while(s[i] != '/' && i < strlen(s)){
        printf("PP i: %d\n",i );

        i += 1;
    }
    return i;
}

char findtypeofFileOrDir(struct ext2_inode inode, char name[EXT2_NAME_LEN]){

    if(EXT2_S_IFREG & inode.i_mode){
        return 'f';
    }
    else if(EXT2_S_IFLNK & inode.i_mode){
        return 'l';
    }
    else if(EXT2_S_IFDIR & inode.i_mode){
        return 'd';
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
             printf("   NAME = %.*s\n", directory->name_len, directory->name);
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
int getfirstBrackets(char *s){
      int i = 0;
      while(s[i] == '/' && i < strlen(s)){
          i += 1;
      }
      return i;
  }
// returns inode holding second last file/dir in path
// "/foo/bar/blah/csc/369/haider/" => returns inode of dir 369
int traverse(char *s, struct ext2_group_desc *bg){
      int length = strlen(s);
      char *copy = malloc(100);
      strncpy(copy, s, length);
      int i = 0;
      int x = 0;
      printf("  ----copy: %s\n", copy);

      char type;
      unsigned char *locatedir = disk + 1024*bg->bg_inode_table + 128;// start at root
      struct ext2_inode  *inode;
      int newinodenum = 1;
      while(i < length ){
          char *dname = malloc(EXT2_NAME_LEN);
          printf("  ----copy in loop: %s\n", copy);
          x = getfirstBrackets(copy);
          i = i + x;
          if(x == 0){
              break;
          }

          printf("  ----x before = %d\n", x);
          copy = copy + x;
          printf("copy: %s\n", copy);
          x = GetFirstDirName(copy);

          printf("  ----x after  = %d\n", x);
          if(x == 0){
              break;
          }
          printf("  ----x after getfirst");
          strncpy(dname, copy, x);

          copy = copy + x;
          printf("  ----filename : %s\n", dname);

          newinodenum = findInode(locatedir, dname);
          if(newinodenum == -1){
              return -1;
          }
          locatedir = disk + 1024*bg->bg_inode_table +128*newinodenum;
          inode = (struct ext2_inode *)  locatedir;
          type = findtypeofFileOrDir(*inode, dname);
          printf("  ----type: %c\n",type );
          if(type != 'd'){
    //          printf("notdir");
              return -1;
          }
          free(dname);
          i = i+x;
      }

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
              newdirectory->file_type = newdirectory->file_type & 0;
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
  newdirectory->file_type = newdirectory->file_type & 0;
   newdirectory->file_type = newdirectory->file_type | EXT2_FT_DIR;
  newdirectory->name_len = name_len;

  return 2;

}
int addFile(struct ext2_inode* parent, int parentInodeNum, char *name, int name_len, struct ext2_super_block* sb, struct ext2_group_desc *bg) {
  for(int j = 14; j >= 0; j--){
      if(parent->i_block[j] != 0){
          struct ext2_dir_entry * directory;
          unsigned short length = 0;
          while (length < EXT2_BLOCK_SIZE) {
            directory = (struct ext2_dir_entry *) (disk + 1024*parent->i_block[j] + length);
            if (length + directory->rec_len == EXT2_BLOCK_SIZE) {
              directory->rec_len = 8 + directory->name_len + findOffset(8 + directory->name_len);
              struct ext2_dir_entry * newdirectory = (struct ext2_dir_entry *) (disk + 1024*parent->i_block[j] + (length + directory->rec_len));
              newdirectory->rec_len = 1024 - (length + directory->rec_len);
              strncpy(newdirectory->name, name, name_len);
              newdirectory->inode = parentInodeNum;
              newdirectory->file_type = newdirectory->file_type & 0;
              newdirectory->file_type = newdirectory->file_type | EXT2_FT_REG_FILE;
              newdirectory->name_len = name_len;
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
  newdirectory->file_type = newdirectory->file_type & 0;
  newdirectory->file_type = newdirectory->file_type | EXT2_FT_REG_FILE;
  newdirectory->name_len = name_len;

  return 2;

}
int main(int argc, char **argv) {

    if(argc != 2) {
        fprintf(stderr, "Usage: %s <image file name>\n", argv[0]);
        exit(1);
    }
    int fd = open(argv[1], O_RDWR);

    // int fd = open("filecptest.txt", O_RDWR);
    // struct stat sbstat;
    //
    // fstat(fd, &sbstat);
    //disk = mmap(NULL, sbstat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
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
    printf("copying a file to: \"/////level1///bfile//////////\" \n");


    char *path = "/////level1///";
    int length = strlen(path);

    fd = open("filecptest.txt", O_RDWR);
    struct stat sbstat;
    char *msg = malloc(EXT2_NAME_LEN);
    fstat(fd, &sbstat);
    //mmap(NULL, 128 * 1024, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    char *file = mmap(NULL, sbstat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    printf("size of file: %d\n", sbstat.st_size);
    printf("---------contents of file ----------------\n");
    for(int i = 0; i< sbstat.st_size; i++){
        //printf("%c", file[i]);
        msg[i] = file[i];
    }
    printf("%s, len: %d\n",msg, strlen(msg) );
    printf("---------contents of file ----------------\n");

    printf("copy contents of the file into bfile\n");
    int inodeIndex = traverse(path, bg);
    inode = (struct ext2_inode *) (disk + EXT2_BLOCK_SIZE*bg->bg_inode_table + inodeIndex*128);
    printf("things in bfile (its inode num = %d)\n", inodeIndex+1);
    char type = findtypeofFileOrDir(*inode, "kjk");
    inode_bitmap = (unsigned char *) (disk + 1024*bg->bg_inode_bitmap);
    printf("type :: %c, inode :: %d\n", type, inodeIndex);
    if(type == 'd'){
        inodeIndex = findNextAvailableInode(sb, inode_bitmap);
        printf("newinode %d\n", inodeIndex);
        addFile(inode, inodeIndex, "filecptest.txt", strlen("filecptest.txt"), sb, bg);
        setBitmapPositionToBeOn(sb, inode_bitmap, inodeIndex);
        inode = (struct ext2_inode *) (disk + EXT2_BLOCK_SIZE*bg->bg_inode_table + inodeIndex*128);
    }

    /*
    http://codewiki.wikidot.com/c:struct-stat
    The struct stat is simply a structure with the following fields:

    Field Name	Description
    st_mode	The current permissions on the file.
    st_ino	The inode for the file (note that this number is unique to all files and directories on a Linux System.
    st_dev	The device that the file currently resides on.
    st_uid	The User ID for the file.
    st_gid	The Group ID for the file.
    st_atime	The most recent time that the file was accessed.
    st_ctime	The most recent time that the file's permissions were changed.
    st_mtime	The most recent time that the file's contents were modified.
    st_nlink	The number of links that there are to this file.
    st_size
    */
    unsigned char *f;
    struct ext2_dir_entry * directory;
    int j = 0;


    block_bitmap = (unsigned char *) (disk + 1024*bg->bg_block_bitmap);
    int tot =  sbstat.st_size;
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


        // strncpy(directory->name, file , sbstat.st_size);
        memmove(directory, file + EXT2_BLOCK_SIZE*j, EXT2_BLOCK_SIZE);
        printf("msg in current block: %s\n", f);
        printf("after moving Inode: %d rec_len: %d name_len: %d type= u name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);

        //directory->name_len = sbstat.st_size;


        printf("j: %d\n",j );
        j++;
        tot = tot - j*EXT2_BLOCK_SIZE;
        printf("tot %d\n", tot);


    }
    // j == 12, first level of indirection
    if(j*1024 <= sbstat.st_size){
        directory = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE*inode->i_block[j]);
        //...
        j++;
    }
    // j == 13, first level of indirection
    if(j*1024 <= sbstat.st_size){
        directory = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE*inode->i_block[j]);
        //...
        j++;
    }
    // j == 14, first level of indirection
    if(j*1024 <= sbstat.st_size){
        directory = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE*inode->i_block[j]);
        //...
        j++;
    }




    j = 0;
   while(j < 12){
       if(inode->i_block[j] != 0){
           directory = (struct ext2_dir_entry *) (disk + EXT2_BLOCK_SIZE*inode->i_block[j]);
           printf("    DIR BLOCK NUM: %d (for inode %d)\n", inode->i_block[j], 12);

           if(directory->file_type & EXT2_FT_REG_FILE){
               printf("Inode: %d rec_len: %d name_len: %d type= f name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);
           }
           else if(directory->file_type & EXT2_FT_DIR){
               printf("Inode: %d rec_len: %d name_len: %d type= d name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);
           }
           else if(directory->file_type & EXT2_FT_SYMLINK){
               printf("Inode: %d rec_len: %d name_len: %d type= l name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);
           }
           else{
               printf("Inode: %d rec_len: %d name_len: %d type= u name=%.*s\n", directory->inode, directory->rec_len, directory->name_len,directory->name_len, directory->name);
           }
       }
       j++;
   }





    return 0;
}
