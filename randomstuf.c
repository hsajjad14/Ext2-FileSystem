/******************************************************************************

                            Online C Compiler.
                Code, Compile, Run and Debug C program online.
Write your code in this editor and press "Run" button to compile and execute it.

*******************************************************************************/
#include <string.h>
#include <stdio.h>
int getfirst(char *s){
    int i = 1;
    while(s[i] != '/' && i < sizeof(s)){

        i += 1;
    }
    return i;

}

int main()
{
    char *a = "/foo/bar/blah";
    int i = 0;
    int length = sizeof(a);
    int x;
    char arr[100];
    printf("length: %d\n", length);
    while(i<=length){
        x = getfirst(a);
        if(x==1){
            break;
        }
        strncpy(arr, a+1,x-1);
        arr[x-1] = '\0';
        printf("%s, %d, %d\n",arr, x,i );

        a = a+x;
        i = i + x;
    }



    return 0;
}

-----------------------------------------------
version 2


---------------
/******************************************************************************

                            Online C Compiler.
                Code, Compile, Run and Debug C program online.
Write your code in this editor and press "Run" button to compile and execute it.

*******************************************************************************/
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

int getfirst(char *s){
    int i = 1;
    while(s[i] != '/' && i < sizeof(s)){
        
        i += 1;
    }
    return i;
    
}
char *findNameInPath(char *path, int length, char *filename){
      int lengthbackwards = length-1;
      int lastdashflag = 0;
      if (path[lengthbackwards] == '/'){
          lengthbackwards--;
          lastdashflag = 1;
      }
      
      while(1){
          if(path[lengthbackwards] == '/'){
              break;
          }
          lengthbackwards--;
      }
      char *temp = path + lengthbackwards +1;
      if(lastdashflag == 1){
          strncpy(filename, temp, length-lengthbackwards-1-1);
          filename[length-lengthbackwards-1-1] = '\0';
      }else{
          strncpy(filename, temp, length-lengthbackwards-1);
          filename[length-lengthbackwards-1] = '\0';
      }
      return filename;
      
  }
int main()
{   
    char *a = "/foo/bar/blah/csc/369/haider";
    int i = 0;
    int length = strlen(a);
    
    //printf("lenback: %d, len: %d\n", lengthbackwards, length);
    char *filename = malloc(100);
    findNameInPath(a, length, filename);
    int lengthbackwards = length-1;
    printf("filename %s\n", filename);
    int x;
    char arr[100];
    //printf("length: %d\n", length);
    while(i<lengthbackwards){
        x = getfirst(a);
        strncpy(arr, a+1,x-1);
        arr[x-1] = '\0';
        printf("%s, %d, %d\n",arr, x, i);
        
        a = a+x;
        if(x==length){
            break;
        }
        

        i = i + x;
    }
    
    

    return 0;
}
