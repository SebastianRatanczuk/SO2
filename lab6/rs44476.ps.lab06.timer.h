// PS IS1 321 LAB06
// Sebastian Ratańczuk
// rs44476@zut.edu.pl

#ifndef PS_TIMER_H
#define PS_TIMER_H

#include <pthread.h>
#include <stdlib.h>

static pthread_key_t timerKey;

static pthread_once_t timerOnce = PTHREAD_ONCE_INIT;

static void freeMemory(void *buffer);

static void createTimerKey(void);

void start();

double stop();

#endif //PS_TIMER_H
