// PS IS1 321 LAB08
// Sebastian Ratańczuk
// rs44476@zut.edu.pl

#define _GNU_SOURCE
#define showAttr(attr) printf("getattr -> mq_curmsgs: %ld, mq_msgsize: %ld, mq_maxmsg: %ld\n", attr.mq_curmsgs, attr.mq_msgsize, attr.mq_maxmsg);

#include <fcntl.h>
#include <mqueue.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

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

long SENDED_TASKS = 0;

mqd_t mq;
mqd_t mqd;
char *queue = "/QueueRs44476";
char *returnQueue = "/ReturnQueueRs44476";
char *sharedDataPath = "/data";
char *sharedStatisticsPath = "/stats";
char *semaphorePath = "/sem";

StatisticsData *statisticsData;

SaltAndHash getSaltAndHash(const char *rawSalt) {
    char *copy1 = strdup(rawSalt);
    char *copy = strdup(rawSalt);
    return (SaltAndHash) {
            .rawData=copy1,
            .type=strtok(copy, "$"),
            .salt=strtok(NULL, "$"),
            .hash= strtok(NULL, "$")};
}

char *readPasswordsAndInitialConfig(const char *path, const char *sharedData,
                                    const char *sharedStatistics) {
    int fd;
    char *addr;
    struct stat sb;
    fd = open(path, O_RDONLY);
    fstat(fd, &sb);
    addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    int shmfd = shm_open(sharedData, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(shmfd, sb.st_size);

    char *perm = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
    strcpy(perm, addr);

    int shst = shm_open(sharedStatistics, O_CREAT | O_EXCL | O_RDWR, S_IRUSR | S_IWUSR);
    ftruncate(shst, sizeof(StatisticsData));

    statisticsData = mmap(NULL, sizeof(StatisticsData), PROT_READ | PROT_WRITE, MAP_SHARED, shst, 0);

    StatisticsData initData = (StatisticsData) {.finishedTasks=0, .startedTasks=0, .password=0};
    memcpy(statisticsData, &initData, sizeof(initData));

    return addr;
}

ThreadData *prepareData(char *rawPasswords, long numberOfTasks, const char *rawSalt, const char *sharedDataPath,
                        const char *sharedStatisticsPath, char *semaphorePath, char *returnQueue) {

    SaltAndHash saltAndHash = getSaltAndHash(rawSalt);

    char *hash = saltAndHash.rawData;
    char *salt = saltAndHash.salt;
    int type = atoi(saltAndHash.type);

    long lengthOfPasswordFile = (long) strlen(rawPasswords);
    double lettersPerTask = (double) lengthOfPasswordFile / (double) numberOfTasks;

    ThreadData *threadData = malloc(sizeof(ThreadData) * numberOfTasks);

    for (int i = 0; i < numberOfTasks; ++i) {

        threadData[i] = (ThreadData) {
                .begin = (long) (i * lettersPerTask),
                .end = (long) lettersPerTask * (i + 1),
                .algorithm = type};

        for (int j = 0; j < strlen(hash); ++j) {
            threadData[i].hash[j] = hash[j];
        }

        for (int j = 0; j < strlen(salt); ++j) {
            threadData[i].salt[j] = salt[j];
        }

        for (int j = 0; j < strlen(sharedDataPath); ++j) {
            threadData[i].dataPath[j] = sharedDataPath[j];
        }

        for (int j = 0; j < strlen(sharedStatisticsPath); ++j) {
            threadData[i].statisticsPath[j] = sharedStatisticsPath[j];
        }

        for (int j = 0; j < strlen(semaphorePath); ++j) {
            threadData[i].semaphorePath[j] = semaphorePath[j];
        }

        for (int j = 0; j < strlen(returnQueue); ++j) {
            threadData[i].returnQueue[j] = returnQueue[j];
        }
    }
    return threadData;
}

void *sendTasksOnThreads(void *rawData) {
    DispatcherData *dispatcherData = (DispatcherData *) rawData;
    for (int i = 0; i < dispatcherData->numberOfTasks; ++i) {
        mq_send(dispatcherData->mq, (const char *) &dispatcherData->data[i], sizeof(ThreadData), 2);
        SENDED_TASKS++;
        if (strcmp(statisticsData->password, "\0") != 0) {
            break;
        }
    }
    return NULL;
}

_Noreturn void *receiveAndSend(void *rawData) {

    struct mq_attr attr;
    mq_getattr(mqd, &attr);
    long buffSize = attr.mq_msgsize;
    char *buff = malloc(buffSize);
    int rval;
    while (1) {
        if (attr.mq_curmsgs > 0) {
            rval = mq_receive(mqd, buff, buffSize, NULL);
            printf("%d\n", rval);
            if (rval != -1) {
                mq_send(mq, (const char *) buff, sizeof(ThreadData), 4);
            }
            sleep(1);
        }
    }
}

void *taskController(void *rawData) {
    TaskControllerData *controllerData = (TaskControllerData *) rawData;
    while (statisticsData->finishedTasks != controllerData->numberOfTasks) {
        printf("\nQueued tasks: %ld / %ld\n", SENDED_TASKS, controllerData->numberOfTasks);
        printf("Started tasks %ld\n", statisticsData->startedTasks);
        printf("Finished tasks %ld\n", statisticsData->finishedTasks);
        printf("Tasks in progress %ld\n", statisticsData->startedTasks - statisticsData->finishedTasks);
        sleep(1);
    }
    return NULL;
}

void exitHandler(int no, siginfo_t *info, void *ucontext) {
    printf("\nUnexpected program shutdown\nCleaning ...\n");
    statisticsData->password[0] = 1;
    sleep(1);
    mq_close(mq);
    mq_unlink(queue);

    shm_unlink(sharedDataPath);
    shm_unlink(sharedStatisticsPath);
    sem_unlink(semaphorePath);
}

int main(int argc, char *argv[]) {
//    "/mnt/c/Users/sebol/CLionProjects/ps/data/lab008.medium.txt";
//    $6$5MfvmFOaDU$CVt7jU9wJRYz3K98EklAJqp8RMG5NvReUSVK7ctVvc2VOnYVrvyTfXaIgHn2xQS78foEJZBq2oCIqwfdNp.2V1

    char *rawSalt = "$6$5MfvmFOaDU$CVt7jU9wJRYz3K98EklAJqp8RMG5NvReUSVK7ctVvc2VOnYVrvyTfXaIgHn2xQS78foEJZBq2oCIqwfdNp.2V1";
    char *filePath = "/mnt/c/Users/sebol/CLionProjects/ps/data/lab008.medium.txt";
    long numberOfTasks = 20;

    int opt;
    while ((opt = getopt(argc, argv, "s:p:t:")) != -1) {
        switch (opt) {
            case 's':
                printf("salt: %s\n", optarg);
                rawSalt = optarg;
                break;
            case 'p':
                printf("file: %s\n", optarg);
                filePath = optarg;
                break;
            case 't':
                numberOfTasks = strtol(optarg, NULL, 0);
                break;
            default:
                break;
        }
    }

    struct sigaction CleanExit;
    CleanExit.sa_sigaction = exitHandler;
    CleanExit.sa_flags = SA_SIGINFO | SA_RESTART;
    sigaction(SIGINT, &CleanExit, NULL);

    printf("Queue name is: %s\n", queue);
    sem_open(semaphorePath, O_RDWR | O_CREAT, 0600, 1);
    char *rawPasswords = readPasswordsAndInitialConfig(filePath, sharedDataPath, sharedStatisticsPath);

    ThreadData *data = prepareData(rawPasswords, numberOfTasks, rawSalt,
                                   sharedDataPath, sharedStatisticsPath, semaphorePath, returnQueue);

    mq = mq_open(queue, O_RDWR | O_CREAT, 0600, NULL);
    mqd = mq_open(returnQueue, O_RDWR | O_CREAT, 0600, NULL);

    printf("queue created");
    pthread_t Dispatcher;
    DispatcherData dispatcherData = (DispatcherData) {
            .data=data,
            .numberOfTasks=numberOfTasks,
            .mq=mq};

    pthread_create(&Dispatcher, NULL, sendTasksOnThreads, &dispatcherData);

    pthread_t Controller;
    TaskControllerData controllerData = (TaskControllerData) {
            .numberOfTasks=numberOfTasks};

    pthread_create(&Controller, NULL, taskController, &controllerData);

    pthread_t QueueReceiver;
    pthread_create(&QueueReceiver, NULL, receiveAndSend, NULL);

    struct mq_attr attr;
    mq_getattr(mq, &attr);

    while (statisticsData->finishedTasks != numberOfTasks) {
        sleep(1);
        if (strcmp(statisticsData->password, "\0") != 0) {
            break;
        }
    }

    mq_close(mq);
    mq_close(mqd);
    mq_unlink(queue);
    mq_unlink(returnQueue);

    shm_unlink(sharedDataPath);
    shm_unlink(sharedStatisticsPath);
    sem_unlink(semaphorePath);

    printf("\nQueued tasks: %ld / %ld\n", SENDED_TASKS, numberOfTasks);
    printf("Started tasks %ld\n", statisticsData->startedTasks);
    printf("Finished tasks %ld\n", statisticsData->finishedTasks);
    printf("Tasks in progress %ld\n", statisticsData->startedTasks - statisticsData->finishedTasks);

    if (strcmp(statisticsData->password, "\0") != 0 && statisticsData->password[0] != 1) {
        printf("Password is: %s\n", statisticsData->password);
    } else {
        printf("Password not found\n");
    }
    return 0;
}
