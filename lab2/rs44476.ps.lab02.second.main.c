// PS IS1 321 LAB02
// Sebastian Ratańczuk
// rs44476@zut.edu.pl 

#include <stdlib.h>
#include <pwd.h>
#include <unistd.h>
#include <utmp.h>
#include <stdio.h>

void who(struct utmp*);

int main(int argc, char *argv[])
{
    struct utmp* utmp = getutent();    
    who(utmp);
}