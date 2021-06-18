// PS IS1 321 LAB06
// Sebastian Ratańczuk
// rs44476@zut.edu.pl

#include "timer.h"

static void freeMemory(void *buffer) {
    free(buffer);
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

