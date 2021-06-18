// PS IS1 321 LAB05
// Sebastian Ratańczuk
// rs44476@zut.edu.pl

#include <stdlib.h>
#include <unistd.h>
#include <utmpx.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>

#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>

#define CHILD_PID 0

int NEXT_GEN_LOCK = 1;
int CHILD_LOCK = 1;
int MAIN_LOCK = 1;
unsigned int FINISHED_CHILDEN = 0;

//https://stackoverflow.com/questions/8398298/handling-multiple-sigchld
//https://www.gnu.org/software/libc/manual/html_node/Merged-Signals.html

void handlerChildReturn(int no, siginfo_t *info, void *ucontext)
{
    // Oryginalny pomysl
    FINISHED_CHILDEN++;
    time_t t;
    time(&t);
    printf("\t\t\t\t%d [%d] [%d] %s", FINISHED_CHILDEN, info->si_pid, info->si_status, ctime(&t));
}

void handlerNextGenLock(int no, siginfo_t *info, void *ucontext)
{
    NEXT_GEN_LOCK = 0;
}

void handlerMainLoop(int no, siginfo_t *info, void *ucontext)
{
    MAIN_LOCK = 0;
}

void handlerChildLock(int no, siginfo_t *info, void *ucontext)
{
    CHILD_LOCK = 0;
}

int main(int argc, char *argv[])
{
    int opt;
    int maxProcessLife = 1;
    int idleTime = 1;

    while ((opt = getopt(argc, argv, "m:w:")) != -1)
    {
        switch (opt)
        {
        case 'm':
            maxProcessLife = atoi(optarg);
            break;
        case 'w':
            idleTime = atoi(optarg);
            break;
        default:
            break;
        }
    }

    struct sigaction ChildReturn;
    ChildReturn.sa_sigaction = handlerChildReturn;
    ChildReturn.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGCHLD, &ChildReturn, NULL);

    struct sigaction ParrentNextGenLock;
    ParrentNextGenLock.sa_sigaction = handlerNextGenLock;
    ParrentNextGenLock.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGALRM, &ParrentNextGenLock, NULL);

    struct sigaction ParrentMainLoop;
    ParrentMainLoop.sa_sigaction = handlerMainLoop;
    ParrentMainLoop.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGINT, &ParrentMainLoop, NULL);

    unsigned int ChildCounter = 0;

    //TEST
    int TMP = maxProcessLife;

    while (MAIN_LOCK)
    {
        ChildCounter++;
        pid_t pid = fork();

        if (pid == CHILD_PID)
        {
            srand(time(NULL));

            int n = rand() % maxProcessLife + 1;
            // n = TMP;
            struct sigaction ChildSIG;
            ChildSIG.sa_sigaction = handlerChildLock;
            ChildSIG.sa_flags = SA_SIGINFO | SA_RESTART;

            sigaction(SIGALRM, &ChildSIG, NULL);

            alarm(n);

            time_t t;
            time(&t);
            printf("%d [%d] [%d] %s", ChildCounter, getpid(), n, ctime(&t));

            unsigned long long int t1 = 0, t2 = 1, nextTerm;
            while (CHILD_LOCK)
            {
                for (size_t i = 0; i < 100000; i++)
                {
                    nextTerm = t1 + t2;
                    t1 = t2;
                    t2 = nextTerm;
                }
            }

            printf("%d\n", n);
            return n;
        }
        else
        {
            // TMP--;
            alarm(idleTime);

            while (NEXT_GEN_LOCK)
                ;

            NEXT_GEN_LOCK = 1;
        }
    }

    while (waitpid(-1, NULL, WNOHANG) != -1)
        ;
    // while (ChildCounter > FINISHED_CHILDEN);

    return 0;
}