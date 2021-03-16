// PS IS1 321 LAB03
// Sebastian Ratańczuk
// rs44476@zut.edu.pl 

#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <utmpx.h>
#include <stdio.h>
#include <grp.h>

struct GroupList
{
    gid_t *groups;
    int numberOfGroups;
};

void printGroup(struct GroupList);

void printHost(struct utmpx* user){
     
    printf("(%s)\t",user->ut_host);
}

struct GroupList getGroup(struct utmpx* user){

    struct GroupList groupList;
    groupList.numberOfGroups = 0;
    struct passwd *paswd;    

    paswd = getpwnam(user->ut_user);
    groupList.groups = malloc(sizeof(*groupList.groups) * groupList.numberOfGroups);
    
    if (getgrouplist(user->ut_user, paswd->pw_gid, groupList.groups, &groupList.numberOfGroups) == -1) {

        free(groupList.groups);
        groupList.groups = malloc(sizeof(*groupList.groups) * groupList.numberOfGroups);

        if (getgrouplist(user->ut_user, paswd->pw_gid, groupList.groups, &groupList.numberOfGroups) == -1) {
            exit(EXIT_FAILURE);
        } 
    }  
    
    return groupList;
}

void printGroup(struct GroupList list){

    struct group *group;
    printf("[");
    for (int j = 0; j < list.numberOfGroups; j++) {
        group = getgrgid(list.groups[j]);
        if (group != NULL)
            printf("%s", group->gr_name);
        printf(", ");
    }
    printf("]");
    free(list.groups);
}
