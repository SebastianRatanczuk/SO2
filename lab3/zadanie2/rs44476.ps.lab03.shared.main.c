// PS IS1 321 LAB03
// Sebastian Ratańczuk
// rs44476@zut.edu.pl 

#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <utmpx.h>
#include <stdio.h>
#include <grp.h>
#include <dlfcn.h>

int hflag=0;
int gflag=0;
void *handle;

int isUser(struct utmpx*);
void printUser(struct utmpx*);
void (*printHost)(struct utmpx*);
void (*getGroup)(struct utmpx*);
void (*printGroup)(gid_t * ,int);



void who(){
    struct utmpx* utmp = getutxent();        
    while(utmp)
    {    
        if(isUser(utmp)) {            
            
            printUser(utmp);

            if(hflag)
                printHost(utmp);

            if(gflag)
                getGroup(utmp);

            printf("\n");                
        }
        utmp = getutxent();        
    }
}

int isUser(struct utmpx* account){

    if(account->ut_type == USER_PROCESS)
        return 1;
    else
        return 0;
}

void printUser(struct utmpx* user){

    printf("%s\t",user->ut_user); 
}

int main(int argc, char *argv[])
{    
    handle = dlopen("./rs44476.ps.lab03.shared.lib.so.0.1", RTLD_LAZY);

    int opt;  

    if(handle){
        while ((opt = getopt(argc, argv, "gh")) != -1) 
        {
            switch (opt) {
            case 'h':
                dlerror();

                printHost=dlsym(handle,"printHost");

                if(!dlerror())
                    hflag = 1;
                break;
            case 'g':
                dlerror();        

                getGroup = dlsym(handle,"getGroup");
                
                if(!dlerror())
                    gflag = 1;
                break;
            default:  
                break;                 
            }
        }    
    }
    
    who();

    if(handle)
        dlclose(handle);
}