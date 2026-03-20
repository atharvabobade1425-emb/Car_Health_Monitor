/*
 * signals/signal_handler.c
 */

#include "signal_handler.h"
#include <sys/wait.h>

void handle_sigint(int sig)
{
    (void)sig;
    const char *shutdown_msg = "SHUTDOWN\n";
    if (pipefd[1] >= 0) write(pipefd[1], shutdown_msg, strlen(shutdown_msg));

    printf("\n[System] Shutting down...\n");
    fflush(stdout);

    if (sensor_pid    > 0) kill(sensor_pid,    SIGTERM);
    if (aggregator_pid > 0) kill(aggregator_pid, SIGTERM);
    if (decision_pid  > 0) kill(decision_pid,  SIGTERM);
    if (logger_pid    > 0) kill(logger_pid,    SIGTERM);

    sleep(1);
    while (waitpid(-1, NULL, WNOHANG) > 0);

    sem_unlink(SEM_NAME);
    unlink(FIFO_PATH);

    printf("[System] All processes terminated. Goodbye.\n");
    fflush(stdout);
    exit(0);
}

void handle_sigusr1(int sig)
{
    (void)sig;
    printf("\n[System] ── Process Snapshot ──────────────────\n");
    printf("  Supervisor     PID: %d\n", getpid());
    printf("  Sensor Manager PID: %d\n", sensor_pid);
    printf("  Aggregator     PID: %d\n", aggregator_pid);
    printf("  Decision Engine PID: %d\n", decision_pid);
    printf("  Logger         PID: %d\n", logger_pid);
    printf("────────────────────────────────────────────────\n\n");
    fflush(stdout);
}

void check_temperature(void)
{
    srand((unsigned int)time(NULL));
    int temp = rand() % 50 + 10;
    if (temp > 30) raise(SIGUSR2);
}

void handle_sigalrm(int sig)
{
    (void)sig;
    printf("[Health Check] System running normally\n");
    fflush(stdout);
    alarm(10);
    check_temperature();
}

void handle_sigusr2(int sig)
{
    (void)sig;
    printf("*** CRITICAL SENSOR ALERT — check log for details ***\n");
    fflush(stdout);
}

void setup_signal_handlers(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));

    sa.sa_handler = handle_sigint;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    sa.sa_handler = handle_sigusr1;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR1, &sa, NULL);

    sa.sa_handler = handle_sigalrm;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGALRM, &sa, NULL);

    sa.sa_handler = handle_sigusr2;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    sigaction(SIGUSR2, &sa, NULL);

    alarm(10);
}
