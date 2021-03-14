// PS IS1 321 LAB03
// Sebastian Ratańczuk
// rs44476@zut.edu.pl 

#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <utmpx.h>
#include <stdio.h>
#include <grp.h>

int hflag=0;
int gflag=0;

int isUser(struct utmpx*);
void printUser(struct utmpx*);
void printHost(struct utmpx*);
void getGroup(struct utmpx*);
void printGroup(gid_t * ,int);

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

void printHost(struct utmpx* user){
     
    printf("(%s)\t",user->ut_host);
}

void getGroup(struct utmpx* user){

    int numberOfGroups = 0;
    struct passwd *paswd;    

    paswd = getpwnam(user->ut_user);
    gid_t *groups = malloc(sizeof(*groups) * numberOfGroups);
    
    if (getgrouplist(user->ut_user, paswd->pw_gid, groups, &numberOfGroups) == -1) {

        free(groups);
        groups = malloc(sizeof(*groups) * numberOfGroups);

        if (getgrouplist(user->ut_user, paswd->pw_gid, groups, &numberOfGroups) == -1) {
            exit(EXIT_FAILURE);
        } 
    }  
    
    printGroup(groups,numberOfGroups);
}

void printGroup(gid_t * groups,int numberOfGroup){

    struct group *group;
    printf("[");
    for (int j = 0; j < numberOfGroup; j++) {
        group = getgrgid(groups[j]);
        if (group != NULL)
            printf("%s", group->gr_name);
        printf(", ");
    }
    printf("]");
    free(groups);
}

int main(int argc, char *argv[])
{    
    int opt;    
    
    while ((opt = getopt(argc, argv, "gh")) != -1) {
        switch (opt) {
        case 'h':
            hflag = 1;
            break;
        case 'g':                   
            gflag = 1;
            break;
        default:  
            break;                 
        }
    }
    
    who();
}