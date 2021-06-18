// PS IS1 321 LAB08
// Sebastian Ratańczuk
// rs44476@zut.edu.pl

#define _GNU_SOURCE

#include <fcntl.h>
#include <mqueue.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <crypt.h>

#define MAX_SIZE 256

typedef struct ThreadData ThreadData;
struct ThreadData {
    long begin;
    long end;

    int algorithm;
    char salt[MAX_SIZE];
    char hash[MAX_SIZE];

    char dataPath[MAX_SIZE];
    char statisticsPath[MAX_SIZE];
    char semaphorePath[MAX_SIZE];
    char returnQueue[MAX_SIZE];
};

void showThreadData(ThreadData data) {
    printf("%ld\n", data.begin);
    printf("%ld\n", data.end);
    printf("%s\n", data.salt);
    printf("%s\n", data.hash);
    printf("%d\n", data.algorithm);
}

typedef struct SaltAndHash SaltAndHash;
struct SaltAndHash {
    char *rawData;
    char *type;
    char *salt;
    char *hash;
};

void showSaltAndHash(SaltAndHash data) {
    printf("%s\n", data.rawData);
    printf("%s\n", data.type);
    printf("%s\n", data.salt);
    printf("%s\n", data.hash);
}

typedef struct DispatcherData DispatcherData;
struct DispatcherData {
    ThreadData *data;
    mqd_t mq;
    long numberOfTasks;
};

typedef struct TaskControllerData TaskControllerData;
struct TaskControllerData {
    long numberOfTasks;
};

typedef struct StatisticsData StatisticsData;
struct StatisticsData {
    long startedTasks;
    long finishedTasks;

    char password[MAX_SIZE];
};

ThreadData taskData;
StatisticsData *statistics;
sem_t *sem;
mqd_t mq;
mqd_t mqd;
int panicSwitch = 1;

char *prepareSalt(ThreadData data) {
    char *type = malloc(4);
    type[0] = 0;
    char *salt;

    char str[50];
    sprintf(str, "%d", data.algorithm);

    strcat(type, "$");
    strcat(type, str);
    strcat(type, "$");

    salt = malloc(strlen(type) + strlen(data.salt) + 1);
    salt[0] = 0;
    strcat(salt, type);
    strcat(salt, data.salt);
    return salt;
}

void runTask(const char *buff) {
    taskData = *(ThreadData *) buff;

    long startOffset = taskData.begin;
    long stopOffset = taskData.end;
    char *dataPath = taskData.dataPath;
    char *statisticsPath = taskData.statisticsPath;
    char *semaphorePath = taskData.semaphorePath;
    char *returnQueue = taskData.returnQueue;

    sem = sem_open(semaphorePath, O_RDWR | O_CREAT, 0600, 1);
    mqd = mq_open(returnQueue, O_RDWR, 0666, NULL);

    struct stat statData, statStat;
    int shmfd = shm_open(dataPath, O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    fstat(shmfd, &statData);
    char *passwords = mmap(NULL, statData.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);

    int shst = shm_open(statisticsPath, O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    fstat(shst, &statStat);
    statistics = mmap(NULL, statStat.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, shst, 0);

    sem_wait(sem);
    statistics->startedTasks++;
    sem_post(sem);

    char *passwordsClone = strdup(passwords);
    unsigned long numbers = 0;

    char *password = strtok(passwordsClone + startOffset, "\n");
    struct crypt_data *cryptData = malloc(sizeof(struct crypt_data));

    while (password != NULL && panicSwitch) {

        if (strcmp(statistics->password, "\0") != 0) {
            if (statistics->password[0] == 1) {
                printf("Search stopped by server\n");
                exit(0);
            } else {
                printf("Password found by another worker. Password is: %s\n", statistics->password);
                exit(0);
            }
        }
        numbers += strlen(password);

        cryptData->initialized = 0;
        char *salt = prepareSalt(taskData);
        char *to_return = crypt_r(password, salt, cryptData);

        if (strcmp(to_return, taskData.hash) == 0) {
            printf("Password is %s\n", password);

            for (int i = 0; i < strlen(password); ++i) {
                statistics->password[i] = password[i];
            }
            break;
        }

        if (numbers > stopOffset - startOffset) {
            break;
        }
        password = strtok(NULL, "\n");
    }

    if (strcmp(statistics->password, "\0") == 0)
        printf("Password not found in this batch\n");

    if (!panicSwitch)
        return;

    sem_wait(sem);
    statistics->finishedTasks++;
    sem_post(sem);

    sem_close(sem);

    free(cryptData);
}

void exitHandler(int no, siginfo_t *info, void *ucontext) {
    printf("\nUnexpected shutdown\nSending data to server ...\n");
    sem_wait(sem);
    statistics->startedTasks--;
    sem_post(sem);
    sem_close(sem);

    mq_send(mqd, (const char *) &taskData, sizeof(ThreadData), 4);

    printf("Data send\n");
    panicSwitch = 0;
}

int main(int argc, char *argv[]) {
    char *queue = "/QueueRs44476";
    long tasksToRun = 1;

    int opt;
    while ((opt = getopt(argc, argv, "s:t:")) != -1) {
        switch (opt) {
            case 's':
                printf("salt: %s\n", optarg);
                queue = optarg;
                break;
            case 't':
                tasksToRun = strtol(optarg, NULL, 0);
                break;
            default:
                break;
        }
    }

    struct sigaction CleanExit;
    CleanExit.sa_sigaction = exitHandler;
    CleanExit.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGINT, &CleanExit, NULL);
    sigaction(SIGILL, &CleanExit, NULL);


    mq = mq_open(queue, O_RDWR, 0666, NULL);

    if (mq == -1) {
        printf("Server not started\n");
        return 0;
    }

    struct mq_attr attr;

    mq_getattr(mq, &attr);
    if (attr.mq_curmsgs == 0) {
        printf("No Jobs\n");
        return 0;
    }

    long buffSize = attr.mq_msgsize;
    char *buff = malloc(buffSize);

    for (long i = 0; i < tasksToRun; ++i) {
        int rval = mq_receive(mq, buff, buffSize, NULL);

        if (rval == -1) {
            printf("Server error\n");
            return (0);
        }

        runTask(buff);

        if (panicSwitch == 0)
            return (0);

        mq_getattr(mq, &attr);
        if (attr.mq_curmsgs == 0) {
            break;
        }
    }

    mq_close(mq);
    return 0;
}
