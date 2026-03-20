/*
 * src/sensor_manager.c
 * Creates shared memory and POSIX semaphore, spawns 3 sensor threads.
 */

#include "common.h"
#include "threads.h"
#include <pthread.h>

volatile sig_atomic_t running = 1;
sem_t *sem = NULL;

static void handle_sigterm(int sig)
{
    (void)sig;
    running = 0;
}

int main(void)
{
    alarm(0);
    signal(SIGALRM, SIG_DFL);

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handle_sigterm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGTERM, &sa, NULL);

    /* Create POSIX named semaphore */
    sem_unlink(SEM_NAME);
    sem = sem_open(SEM_NAME, O_CREAT, 0644, 1);
    if (sem == SEM_FAILED) {
        perror("[SensorManager] sem_open");
        exit(EXIT_FAILURE);
    }

    /* Create shared memory */
    int shmid = shmget(SHM_KEY, sizeof(sensor_data), IPC_CREAT | 0666);
    if (shmid < 0) {
        perror("[SensorManager] shmget");
        sem_close(sem);
        sem_unlink(SEM_NAME);
        exit(EXIT_FAILURE);
    }

    sensor_data *data = (sensor_data *)shmat(shmid, NULL, 0);
    if (data == (sensor_data *)-1) {
        perror("[SensorManager] shmat");
        sem_close(sem);
        sem_unlink(SEM_NAME);
        exit(EXIT_FAILURE);
    }

    /* Safe initial values */
    data->pressure  = 30;
    data->alignment = 20;
    data->oil_level = 50;

    /* Spawn 3 sensor threads */
    pthread_t tid_pressure, tid_alignment, tid_oil;

    if (pthread_create(&tid_pressure,  NULL, pressure_thread,  data) != 0) {
        perror("[SensorManager] pthread_create pressure");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid_alignment, NULL, alignment_thread, data) != 0) {
        perror("[SensorManager] pthread_create alignment");
        exit(EXIT_FAILURE);
    }
    if (pthread_create(&tid_oil,       NULL, oil_level_thread, data) != 0) {
        perror("[SensorManager] pthread_create oil");
        exit(EXIT_FAILURE);
    }

    /* Join threads — they exit when running == 0 */
    pthread_join(tid_pressure,  NULL);
    pthread_join(tid_alignment, NULL);
    pthread_join(tid_oil,       NULL);

    /* Cleanup */
    shmdt(data);
    shmctl(shmid, IPC_RMID, NULL);
    sem_close(sem);
    sem_unlink(SEM_NAME);

    return 0;
}
