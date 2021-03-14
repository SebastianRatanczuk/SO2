// PS IS1 321 LAB02
// Sebastian Ratańczuk
// rs44476@zut.edu.pl 

#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <utmp.h>
#include <stdio.h>

void who(struct utmp* utmp)
{
    while(utmp)
    {    
        if(utmp->ut_type == USER_PROCESS)
        {
            struct passwd* passwdDB = getpwnam(utmp->ut_user);   

            printf("%s\t",utmp->ut_user); 
            printf("%d\n",passwdDB->pw_uid);
        }

        utmp = getutent();        
    }
}