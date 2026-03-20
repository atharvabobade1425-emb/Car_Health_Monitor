/*
 * threads/alignment_thread.c
 * Simulates wheel alignment sensor. Range: 10-50 degrees. Normal: < 40.
 */

#include "threads.h"

extern volatile sig_atomic_t running;
extern sem_t *sem;

void *alignment_thread(void *arg)
{
    sensor_data *data = (sensor_data *)arg;
    srand((unsigned int)(time(NULL) ^ (unsigned long)pthread_self()));

    while (running) {
        int alignment = rand() % (50 - 10 + 1) + 10;

        sem_wait(sem);
        data->alignment = alignment;
        sem_post(sem);

        sleep(2);
    }

    return NULL;
}
