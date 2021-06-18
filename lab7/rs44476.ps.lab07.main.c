// PS IS1 321 LAB07
// Sebastian Ratańczuk
// rs44476@zut.edu.pl

#define _GNU_SOURCE
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <fcntl.h>
#include <crypt.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <errno.h>

static pthread_key_t lockKey;
static pthread_once_t lockOnce = PTHREAD_ONCE_INIT;
static pthread_key_t timerKey;
static pthread_once_t timerOnce = PTHREAD_ONCE_INIT;

static void freeMemory(void *buffer) {
    free(buffer);
}

static void createLockKey(void) {
    pthread_key_create(&lockKey, freeMemory);
}

int Lock() {
    int *lock;
    pthread_once(&lockOnce, createLockKey);
    lock = pthread_getspecific(lockKey);
    if (lock == NULL) {
        lock = malloc(sizeof(unsigned long));
        *lock = 1;
        pthread_setspecific(lockKey, lock);
    }

    return *lock;
}

void UnLock() {
    int *lock;
    pthread_once(&lockOnce, createLockKey);
    lock = pthread_getspecific(lockKey);
    if (lock == NULL) {
        lock = malloc(sizeof(unsigned long));
        *lock = 1;
        pthread_setspecific(lockKey, lock);
    } else {
        *lock = 0;
        pthread_setspecific(lockKey, lock);
    }
}

static void createTimerKey(void) {
    pthread_key_create(&timerKey, freeMemory);
}

void start() {
    double *timer;
    pthread_once(&timerOnce, createTimerKey);

    timer = pthread_getspecific(timerKey);
    if (timer == NULL) {
        timer = malloc(sizeof(double));
    }

    struct timespec t1;
    clock_gettime(CLOCK_REALTIME, &t1);

    *timer = t1.tv_sec + t1.tv_nsec / 1e9;
    pthread_setspecific(timerKey, timer);
}

double stop() {
    double *timer;
    pthread_once(&timerOnce, createTimerKey);

    timer = pthread_getspecific(timerKey);
    if (timer == NULL) {
        return 0;
    } else {
        struct timespec t1;
        clock_gettime(CLOCK_REALTIME, &t1);

        double time = t1.tv_sec + t1.tv_nsec / 1e9;
        return time - *timer;
    }
}

typedef struct FileByRows FileByRows;
struct FileByRows {
    long numberOfRows;
    char **file;
};

typedef struct SaltAndHash SaltAndHash;
struct SaltAndHash {
    char *rawData;
    char *type;
    char *salt;
    char *hash;
};

typedef struct ThreadData ThreadData;
struct ThreadData {
    FileByRows fileByRows;
    SaltAndHash saltAndHash;
};

long globalProgressMax = 0;
long globalProgress = 0;
long numberOfThreads = 0;

pthread_t watchDogPthread;
pthread_t *bruteForceThreads;
pthread_mutex_t fastMutex = PTHREAD_MUTEX_INITIALIZER;

void handle_error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static char *hashPassword(SaltAndHash saltAndPassword) {
    struct crypt_data* data = malloc(sizeof(struct crypt_data));
    data->initialized=0;

    char *type = malloc(4);
    type[0]=0;
    char *salt;

    strcat(type, "$");
    strcat(type, saltAndPassword.type);
    strcat(type, "$");

    salt = malloc(strlen(type) + strlen(saltAndPassword.salt) + 1);
    salt[0]=0;
    strcat(salt, type);
    strcat(salt, saltAndPassword.salt);

    char *to_return = crypt_r(saltAndPassword.hash, salt, data);
    return to_return;
}

SaltAndHash getSaltAndHash(const char *rawSalt) {
    char *copy1 = strdup(rawSalt);
    char *copy = strdup(rawSalt);
    return (SaltAndHash) {.rawData=copy1, .type=strtok(copy, "$"),
            .salt=strtok(NULL, "$"), .hash= strtok(NULL, "$")};
}

FileByRows readFile(const char *path, int maxSize) {
    char *addr;
    int fd;
    struct stat sb;

    fd = open(path, O_RDONLY);
    if (fd == -1)
        handle_error("open");

    if (fstat(fd, &sb) == -1)
        handle_error("fstat");

    addr = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);

    if (addr == MAP_FAILED)
        handle_error("mmap");

    char *copy1 = strdup(addr);
    char *copy2 = strdup(addr);

    munmap(addr, sb.st_size);
    close(fd);

    char *token = strtok(copy1, "\n");

    long numberOfRows = 0;

    while (token != NULL) {
        numberOfRows++;
        token = strtok(NULL, "\n");
    }

    if (maxSize)
        if (numberOfRows > maxSize)
            numberOfRows = maxSize;

    char **fileByRows = malloc(sizeof(char *) * numberOfRows);

    fileByRows[0] = strtok(copy2, "\n");
    for (int i = 1; i < numberOfRows; ++i) {
        fileByRows[i] = strtok(NULL, "\n");
    }

    return (FileByRows) {.numberOfRows = numberOfRows, .file = fileByRows};
}

ThreadData *prepareDataForThreads(FileByRows fileByRows, SaltAndHash saltAndHash) {
    long chunkSize = fileByRows.numberOfRows / numberOfThreads;

    ThreadData *threadData = malloc(sizeof(ThreadData) * numberOfThreads);

    for (long i = 0; i < numberOfThreads; i++) {
        long start = i * chunkSize;
        long end = start + chunkSize - 1;

        if (i == numberOfThreads - 1) {
            end = fileByRows.numberOfRows - 1;
        }

        char **singleThreadData = malloc(sizeof(char *) * (end - start + 1));
        long counter = 0;

        for (long j = start; j <= end; j++) {
            singleThreadData[counter++] = fileByRows.file[j];
        }

        FileByRows fileByRow = (FileByRows) {.numberOfRows = end - start + 1, .file = singleThreadData};

        threadData[i] = (ThreadData) {.fileByRows = fileByRow, .saltAndHash = saltAndHash};
    }
    return threadData;
}

void *bruteForce(void *rawData) {
    ThreadData *threadData = (ThreadData *) rawData;

    SaltAndHash saltAndHash = threadData->saltAndHash;
    FileByRows fileByRows = threadData->fileByRows;

    long localCounter = 0;

    for (int i = 0; i < fileByRows.numberOfRows; ++i) {

        SaltAndHash saltAndPassword = (SaltAndHash) {.type=saltAndHash.type, .salt=saltAndHash.salt, .hash=fileByRows.file[i]};
        char *rawHash = hashPassword(saltAndPassword);

        if (strcmp(rawHash, saltAndHash.rawData) == 0) {
            printf("Hasło to: %s\n", fileByRows.file[i]);
            pthread_kill(watchDogPthread, SIGUSR1);
            pthread_exit((void *) 1);
        }

        localCounter++;

        if (localCounter > 100) {
            pthread_mutex_lock(&fastMutex);
            globalProgress += localCounter;
            pthread_mutex_unlock(&fastMutex);
            localCounter = 0;
        }
    }

    pthread_mutex_lock(&fastMutex);
    globalProgress += localCounter;
    pthread_mutex_unlock(&fastMutex);

    pthread_exit(0);
}

void handler() {
    UnLock();
}

void *watchDog() {
    signal(SIGUSR1, handler);
    double progress;
    while (Lock()) {
        progress = (double) globalProgress / (double) globalProgressMax;

        int barWidth = 100;
        printf("[");

        int pos = barWidth * progress;

        for (int i = 0; i < barWidth; ++i) {
            if (i < pos) printf("=");
            else if (i == pos) printf(">");
            else printf(" ");
        }
        printf("]");
        printf(" %f%\n", progress * 100);
        fflush(stdout);

        if (progress >= 1) {
            UnLock();
            printf("Hasła nie znaleziono\n");
        }
        sleep(1);
    }
    return NULL;
}

void initializeBruteForce(const char *rawSalt, const char *filePath) {

    SaltAndHash saltAndHash = getSaltAndHash(rawSalt);

    FileByRows fileByRows = readFile(filePath, 0);
    globalProgressMax = fileByRows.numberOfRows;

    ThreadData *threadData = prepareDataForThreads(fileByRows, saltAndHash);
    bruteForceThreads = malloc(sizeof(pthread_t) * numberOfThreads);

    for (int i = 0; i < numberOfThreads; ++i) {
        pthread_create(&bruteForceThreads[i], NULL, bruteForce, &threadData[i]);
    }

    pthread_create(&watchDogPthread, NULL, watchDog, NULL);

    pthread_join(watchDogPthread, NULL);
    pthread_mutex_destroy(&fastMutex);
}

void initializeBenchmark(const char *rawSalt, const char *filePath) {
    SaltAndHash saltAndHash = getSaltAndHash(rawSalt);

    FileByRows fileByRows = readFile(filePath, 10000);
    globalProgressMax = fileByRows.numberOfRows;

    int maxThreads = 4;

    for (int i = 1; i <= maxThreads; ++i) {

        printf("Liczba wątków: %d\n", i);
        globalProgress = 0;

        numberOfThreads = i;
        ThreadData *threadData = prepareDataForThreads(fileByRows, saltAndHash);
        bruteForceThreads = malloc(sizeof(pthread_t) * numberOfThreads);

        start();
        for (int j = 0; j < numberOfThreads; ++j) {
            pthread_create(&bruteForceThreads[j], NULL, bruteForce, &threadData[j]);
        }
        pthread_create(&watchDogPthread, NULL, watchDog, NULL);
        pthread_join(watchDogPthread, NULL);

        printf("Czas: %f\n", stop());
        free(threadData);
        free(bruteForceThreads);
    }

    pthread_mutex_destroy(&fastMutex);
}

int main(int argc, char *argv[]) {
    char *rawSalt = NULL;
    char *filePath = NULL;
    int opt = 0;
    numberOfThreads = 0;
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
                numberOfThreads = strtol(optarg, NULL, 0);
                break;
            default:
                break;
        }
    }

    if (numberOfThreads > get_nprocs_conf())
        numberOfThreads = get_nprocs_conf();

    if (rawSalt == NULL || filePath == NULL) {
        errno = EINVAL;
        handle_error("no file or hash");
    }

    if (numberOfThreads) {
        initializeBruteForce(rawSalt, filePath);
    } else {
        initializeBenchmark(rawSalt, filePath);
    }

    return 0;
}
