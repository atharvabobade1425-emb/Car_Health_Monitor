/*
 * src/main.c
 * Supervisor: creates IPC, forks 4 children, handles signals.
 */

#include "common.h"
#include "signal_handler.h"
#include <sys/wait.h>
#include <sys/stat.h>

pid_t sensor_pid     = -1;
pid_t aggregator_pid = -1;
pid_t decision_pid   = -1;
pid_t logger_pid     = -1;
int   pipefd[2]      = {-1, -1};

static void fork_exec(const char *binary, char *const argv[], pid_t *out_pid)
{
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); exit(EXIT_FAILURE); }
    if (pid == 0) { execv(binary, argv); perror("execv"); exit(EXIT_FAILURE); }
    *out_pid = pid;
}

int main(void)
{
    printf("Smart Car Service Monitoring System started (pid=%d)\n", getpid());
    printf("────────────────────────────────────────────────\n");
    fflush(stdout);

    setup_signal_handlers();

    if (pipe(pipefd) < 0) { perror("pipe"); exit(EXIT_FAILURE); }

    unlink(FIFO_PATH);
    mkfifo(FIFO_PATH, 0666);

    /* Create shared memory and message queue */
    if (shmget(SHM_KEY, sizeof(sensor_data), IPC_CREAT | 0666) < 0) {
        perror("shmget"); exit(EXIT_FAILURE);
    }
    if (msgget(MSG_KEY, IPC_CREAT | 0666) < 0) {
        perror("msgget"); exit(EXIT_FAILURE);
    }

    /* Export fds and supervisor PID to children */
    char pfd0[16], pfd1[16], spid[16];
    snprintf(pfd0, sizeof(pfd0), "%d", pipefd[0]);
    snprintf(pfd1, sizeof(pfd1), "%d", pipefd[1]);
    snprintf(spid, sizeof(spid), "%d", getpid());
    setenv("PIPE_READ",      pfd0, 1);
    setenv("PIPE_WRITE",     pfd1, 1);
    setenv("SUPERVISOR_PID", spid, 1);

    /* Fork children */
    char *args_sm[]  = {"./build/sensor_manager",  NULL};
    char *args_agg[] = {"./build/data_aggregator",  NULL};
    char *args_de[]  = {"./build/decision_engine",  NULL};
    char *args_log[] = {"./build/logger",            NULL};

    fork_exec("./build/sensor_manager",  args_sm,  &sensor_pid);
    fork_exec("./build/data_aggregator", args_agg, &aggregator_pid);
    fork_exec("./build/decision_engine", args_de,  &decision_pid);
    fork_exec("./build/logger",          args_log, &logger_pid);

    /* Supervisor closes both pipe ends after all forks */
    close(pipefd[0]);
    close(pipefd[1]);
    pipefd[0] = pipefd[1] = -1;

    printf("Press Ctrl+C to stop. Snapshot every 5s.\n");
    printf("────────────────────────────────────────────────\n\n");
    fflush(stdout);

    while (1) {
        int status;
        pid_t dead = waitpid(-1, &status, WNOHANG);
        if (dead > 0) {
            /* silent reap */
        }
        pause();
    }

    return 0;
}
