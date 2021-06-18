// PS IS1 321 LAB06
// Sebastian Ratańczuk
// rs44476@zut.edu.pl

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <pthread.h>
#include <time.h>
#include "timer.h"

static pthread_key_t lockKey;
static pthread_once_t lockOnce = PTHREAD_ONCE_INIT;
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

struct ThreadKillerData {
    int sec;
    pthread_t tid;
};

void handler(int sigNo) {
    UnLock();
}

void *threadKill(void *rawData) {
    struct ThreadKillerData *data = (struct ThreadKillerData *) rawData;
    sleep(data->sec);
    pthread_kill(data->tid, SIGINT);
    return NULL;
}

void *fibbonaci(void *unused) {

    struct sigaction act;
    act.sa_handler = handler;
    act.sa_flags = 0;
    sigemptyset(&(act.sa_mask));
    sigaction(SIGINT, &act, NULL);

    unsigned long long int t1 = 0, t2 = 1, nextTerm;

    start();
    while (Lock()) {
        nextTerm = t1 + t2;
        t1 = t2;
        t2 = nextTerm;
    }

    double time = stop();

    printf("\t %ld %f\n", pthread_self(), time);
    return NULL;
}

int main(int argc, char *argv[]) {
    int opt = 0;
    long maxThreadLife = 0;
    long threadCount = 0;

    while ((opt = getopt(argc, argv, "m:c:")) != -1) {
        switch (opt) {
            case 'm':
                maxThreadLife = strtol(optarg, NULL, 0);
                break;
            case 'c':
                threadCount = strtol(optarg, NULL, 0);
                break;
            default:
                break;
        }
    }

    srand(time(NULL) * 2);

    pthread_t *threads = malloc(sizeof(pthread_t) * threadCount);
    pthread_t *threadKillers = malloc(sizeof(pthread_t) * threadCount);
    struct ThreadKillerData *threadKillerData = malloc(sizeof(struct ThreadKillerData) * threadCount);

    for (int i = 0; i < threadCount; ++i) {
        int actualThreadLife = rand() % maxThreadLife + 1;

        pthread_create(&threads[i], NULL, fibbonaci, NULL);

        threadKillerData[i].sec = actualThreadLife;
        threadKillerData[i].tid = threads[i];

        pthread_create(&threadKillers[i], NULL, threadKill, &threadKillerData[i]);

        printf("%lu %d\n", threads[i], actualThreadLife);
    }

    for (int i = 0; i < threadCount; ++i) {
        pthread_join(threads[i], NULL);
        pthread_join(threadKillers[i], NULL);
    }

    free(threads);
    free(threadKillers);
    free(threadKillerData);

    printf("koniec\n");
    return 0;
}