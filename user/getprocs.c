#include "kernel/types.h"
#include "user/user.h"
int main(int argc,char*argv[]){
    int count=getprocs();
    printf("There are %d active processed.\n",count);
    // printf("\n");
    exit(0);

}