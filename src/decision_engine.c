/*
 * src/decision_engine.c
 * Evaluates thresholds, writes WARNING/CRITICAL alerts to pipe -> logger.
 * Raises SIGUSR2 to supervisor on critical conditions.
 */

#include "common.h"

static volatile sig_atomic_t running = 1;
static pid_t supervisor_pid = -1;

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

    /* Get supervisor PID to send signals */
    const char *sup_pid_str = getenv("SUPERVISOR_PID");
    if (sup_pid_str) supervisor_pid = atoi(sup_pid_str);

    /* Pipe setup: close read end, dup write end to stdout */
    const char *pfd1_str = getenv("PIPE_WRITE");
    if (!pfd1_str) { fprintf(stderr, "[DecisionEngine] PIPE_WRITE not set\n"); exit(EXIT_FAILURE); }
    int pfd_write = atoi(pfd1_str);

    const char *pfd0_str = getenv("PIPE_READ");
    if (pfd0_str) close(atoi(pfd0_str));

    if (dup2(pfd_write, STDOUT_FILENO) < 0) { perror("[DecisionEngine] dup2"); exit(EXIT_FAILURE); }
    close(pfd_write);

    /* Open message queue */
    int msgid = msgget(MSG_KEY, 0666);
    if (msgid < 0) { perror("[DecisionEngine] msgget"); exit(EXIT_FAILURE); }

    /* Main loop */
    while (running) {
        message msg;
        memset(&msg, 0, sizeof(msg));

        ssize_t rc = msgrcv(msgid, &msg, sizeof(msg) - sizeof(long), MSG_TYPE_DATA, 0);
        if (rc < 0) {
            if (errno == EINTR) continue;
            if (running) perror("[DecisionEngine] msgrcv");
            break;
        }

        int critical = 0;

        /* ── Tyre Pressure ── */
        if (msg.pressure < PRESSURE_CRIT_LOW) {
            printf("*** CRITICAL ALERT: Tyre pressure dangerously LOW (%d psi) — stop vehicle immediately! ***\n", msg.pressure);
            fflush(stdout);
            critical = 1;
        } else if (msg.pressure > PRESSURE_CRIT_HIGH) {
            printf("*** CRITICAL ALERT: Tyre pressure dangerously HIGH (%d psi) — risk of blowout! ***\n", msg.pressure);
            fflush(stdout);
            critical = 1;
        } else if (msg.pressure < PRESSURE_WARN_LOW) {
            printf("  WARNING: Tyre pressure low (%d psi) — inflate tyres soon.\n", msg.pressure);
            fflush(stdout);
        } else if (msg.pressure > PRESSURE_WARN_HIGH) {
            printf("  WARNING: Tyre pressure high (%d psi) — check inflation.\n", msg.pressure);
            fflush(stdout);
        }

        /* ── Wheel Alignment ── */
        if (msg.alignment > ALIGNMENT_CRIT) {
            printf("*** CRITICAL ALERT: Wheel alignment critical (%d deg) — unsafe to drive! ***\n", msg.alignment);
            fflush(stdout);
            critical = 1;
        } else if (msg.alignment > ALIGNMENT_WARN) {
            printf("  WARNING: Wheel alignment off (%d deg) — service required.\n", msg.alignment);
            fflush(stdout);
        }

        /* ── Oil Level ── */
        if (msg.oil_level < OIL_CRIT) {
            printf("*** CRITICAL ALERT: Oil level dangerously low (%d%%) — engine damage risk! ***\n", msg.oil_level);
            fflush(stdout);
            critical = 1;
        } else if (msg.oil_level < OIL_WARN) {
            printf("  WARNING: Oil level low (%d%%) — top up soon.\n", msg.oil_level);
            fflush(stdout);
        }

        /* Raise SIGUSR2 to supervisor on any critical condition */
        if (critical && supervisor_pid > 0) {
            kill(supervisor_pid, SIGUSR2);
        }
    }

    return 0;
}
