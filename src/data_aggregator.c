/*
 * src/data_aggregator.c
 * Reads shared memory every 5s, sends to decision engine via message queue.
 */

#include "common.h"

static volatile sig_atomic_t running = 1;

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

    /* Open shared memory */
    int shmid = shmget(SHM_KEY, sizeof(sensor_data), 0666);
    if (shmid < 0) {
        perror("[Aggregator] shmget");
        exit(EXIT_FAILURE);
    }

    sensor_data *data = (sensor_data *)shmat(shmid, NULL, SHM_RDONLY);
    if (data == (sensor_data *)-1) {
        perror("[Aggregator] shmat");
        exit(EXIT_FAILURE);
    }

    /* Open semaphore with retry loop */
    sem_t *sem = SEM_FAILED;
    int retries = 10;
    while (retries-- > 0 && sem == SEM_FAILED) {
        sem = sem_open(SEM_NAME, 0);
        if (sem == SEM_FAILED) {
            if (errno == ENOENT) { sleep(1); }
            else { perror("[Aggregator] sem_open"); exit(EXIT_FAILURE); }
        }
    }
    if (sem == SEM_FAILED) {
        fprintf(stderr, "[Aggregator] Could not open semaphore\n");
        exit(EXIT_FAILURE);
    }

    /* Open message queue */
    int msgid = msgget(MSG_KEY, IPC_CREAT | 0666);
    if (msgid < 0) {
        perror("[Aggregator] msgget");
        exit(EXIT_FAILURE);
    }

    /* Main loop */
    while (running) {
        /* Lock -> copy -> unlock */
        sem_wait(sem);
        int snap_alignment = data->alignment;
        int snap_pressure  = data->pressure;
        int snap_oil       = data->oil_level;
        sem_post(sem);

        /* Send to decision engine */
        message msg;
        memset(&msg, 0, sizeof(msg));
        msg.type      = MSG_TYPE_DATA;
        msg.alignment = snap_alignment;
        msg.pressure  = snap_pressure;
        msg.oil_level = snap_oil;

        if (msgsnd(msgid, &msg, sizeof(msg) - sizeof(long), 0) < 0) {
            if (running) perror("[Aggregator] msgsnd");
        }

        /* Print clean snapshot */
        printf("\n[Aggregator] ── Sensor Snapshot ──────────────\n");
        printf("  Wheel Alignment : %d degrees\n", snap_alignment);
        printf("  Tyre Pressure   : %d psi\n",     snap_pressure);
        printf("  Oil Level       : %d%%\n",         snap_oil);
        printf("────────────────────────────────────────────────\n");
        fflush(stdout);

        sleep(5);
    }

    shmdt(data);
    sem_close(sem);
    return 0;
}
