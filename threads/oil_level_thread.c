/*
 * threads/oil_level_thread.c
 * Simulates oil level sensor. Range: 20-100%. Normal: > 25%.
 */

#include "threads.h"

extern volatile sig_atomic_t running;
extern sem_t *sem;

void *oil_level_thread(void *arg)
{
    sensor_data *data = (sensor_data *)arg;
    srand((unsigned int)(time(NULL) ^ (unsigned long)pthread_self()));

    while (running) {
        int oil_level = rand() % (100 - 20 + 1) + 20;

        sem_wait(sem);
        data->oil_level = oil_level;
        sem_post(sem);

        sleep(4);
    }

    return NULL;
}
