// PS IS1 321 LAB04
// Sebastian Ratańczuk
// rs44476@zut.edu.pl

#include <sys/types.h>
#include <sys/resource.h>
#include <sys/wait.h>

#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <utmpx.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#define CHILD_PID 0

int noPrintFlag = 0;
extern char **environ;

void redirectSTDOUT()
{
    close(1);
    close(2);
    int h = open("/dev/null", O_WRONLY);
    dup2(h, 1);
    dup2(h, 2);    
}

struct timeval timespecToTimeval(struct timespec *ts)
{
    struct timeval tv;
    tv.tv_sec = ts->tv_sec;
    tv.tv_usec = ts->tv_nsec / 1000;
    return tv;
}

struct timeval divideTimeval(struct timeval timeval, int divider)
{
    struct timeval dividedTime;
    double ints;

    dividedTime.tv_sec = timeval.tv_sec / divider;
    double number = timeval.tv_sec / (double)divider;

    dividedTime.tv_usec = timeval.tv_usec / divider + (int)(modf(number, &ints) * 1000000);

    return dividedTime;
}

int main(int argc, char *argv[])
{
    int opt;
    long timesToRun = 1;

    while ((opt = getopt(argc, argv, "+vt:")) != -1)
    {
        switch (opt)
        {
        case 't':
            timesToRun = atol(optarg);
            break;
        case 'v':
            noPrintFlag = 1;
            break;
        default:
            break;
        }      
    }

    struct timeval userTimeSum;
    userTimeSum.tv_sec = 0;
    userTimeSum.tv_usec = 0;

    struct timeval systemTimeSum;
    systemTimeSum.tv_sec = 0;
    systemTimeSum.tv_usec = 0;

    struct timeval realTimeSum;
    realTimeSum.tv_sec = 0;
    realTimeSum.tv_usec = 0;

    struct rusage rusage;
    int result;

    struct timespec tp_timespec;
    struct timespec tp2_timespec;

    struct timeval tp_timeval;
    struct timeval tp2_timeval;

    clockid_t clk_id;
    clk_id = CLOCK_REALTIME;

    for (long i = 0; i < timesToRun; i++)
    {
        result = clock_gettime(clk_id, &tp_timespec);
        pid_t pid = fork();

        if (pid == CHILD_PID)
        {
            if (noPrintFlag)
                redirectSTDOUT();

            char *envArgs[] = {"PATH=/bin", "USER=user", NULL};
            environ = envArgs;

            char **programArgs = malloc(sizeof(char *) * (argc - optind + 1));
            programArgs[argc - optind] = NULL;
            
            int counter = 0;

            for (size_t i = optind; i < argc; i++)
            {
                programArgs[counter++] = argv[i];
            }

            execvp(programArgs[0], programArgs);
        }
        else
        {
            wait4(pid, NULL, 0, &rusage);

            result = clock_gettime(clk_id, &tp2_timespec);

            tp_timeval = timespecToTimeval(&tp_timespec);
            tp2_timeval = timespecToTimeval(&tp2_timespec);

            timersub(&tp2_timeval, &tp_timeval, &tp2_timeval);

            timeradd(&realTimeSum, &tp2_timeval, &realTimeSum);
            timeradd(&userTimeSum, &rusage.ru_utime, &userTimeSum);
            timeradd(&systemTimeSum, &rusage.ru_stime, &systemTimeSum);

            printf("Real time:\tseconds : %ld\tmicro seconds : %ld\n",
                   tp2_timeval.tv_sec, tp2_timeval.tv_usec);

            printf("User time:\tseconds : %ld\tmicro seconds : %ld\n",
                   rusage.ru_utime.tv_sec, rusage.ru_utime.tv_usec);

            printf("System time:\tseconds : %ld\tmicro seconds : %ld\n",
                   rusage.ru_stime.tv_sec, rusage.ru_stime.tv_usec);
        }
    }

    if (timesToRun != 1)
    {
        struct timeval meanRealTime = divideTimeval(realTimeSum, timesToRun);
        struct timeval meanUserTime = divideTimeval(userTimeSum, timesToRun);
        struct timeval meanSystemTime = divideTimeval(systemTimeSum, timesToRun);

        printf("Mean real time:\t\tseconds : %ld\tmicro seconds : %ld\n",
               meanRealTime.tv_sec, meanRealTime.tv_usec);

        printf("Mean user time:\t\tseconds : %ld\tmicro seconds : %ld\n",
               meanUserTime.tv_sec, meanUserTime.tv_usec);

        printf("Mean system time:\tseconds : %ld\tmicro seconds : %ld\n",
               meanSystemTime.tv_sec, meanSystemTime.tv_usec);
    }

    return 0;
}
