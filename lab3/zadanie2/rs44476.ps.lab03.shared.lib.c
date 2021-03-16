// PS IS1 321 LAB03
// Sebastian Ratańczuk
// rs44476@zut.edu.pl 

#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <utmpx.h>
#include <stdio.h>
#include <grp.h>

void printGroup(gid_t * ,int );

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
