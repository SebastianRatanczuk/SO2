#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

int main(){

    pid_t pid = fork();

    if(pid<0){
        printf("Error\n");
    }

    if(pid==0){
        printf("I'm parent. My pid is: %d\n",getpid());
    }
    else{
        printf("I'm child. My pid is: %d\n",getpid());
    }

    return 0;
}