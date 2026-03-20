/*
 * threads/pressure_thread.c
 * Simulates tyre pressure sensor. Range: 15-40 psi. Normal: 25-35.
 */

#include "threads.h"

extern volatile sig_atomic_t running;
extern sem_t *sem;

void *pressure_thread(void *arg)
{
    sensor_data *data = (sensor_data *)arg;
    srand((unsigned int)(time(NULL) ^ (unsigned long)pthread_self()));

    while (running) {
        int pressure = rand() % (40 - 15 + 1) + 15;

        sem_wait(sem);
        data->pressure = pressure;
        sem_post(sem);

        sleep(3);
    }

    return NULL;
}
