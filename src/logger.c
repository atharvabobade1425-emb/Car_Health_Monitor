/*
 * src/logger.c
 * Reads alerts from pipe, prints to terminal and logs to file with timestamp.
 */

#include "common.h"

static volatile sig_atomic_t running = 1;

static void handle_sigterm(int sig)
{
    (void)sig;
    running = 0;
}

static void log_alert(FILE *logfp, const char *line)
{
    time_t now = time(NULL);
    char ts[64];
    struct tm *tm_info = localtime(&now);
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm_info);

    /* Print alert to terminal */
    printf("%s\n", line);
    fflush(stdout);

    /* Write to log file with timestamp */
    fprintf(logfp, "[%s] %s\n", ts, line);
    fflush(logfp);
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

    /* Pipe setup: close write end, dup read end to stdin */
    const char *pfd0_str = getenv("PIPE_READ");
    if (!pfd0_str) { fprintf(stderr, "[Logger] PIPE_READ not set\n"); exit(EXIT_FAILURE); }
    int pfd_read = atoi(pfd0_str);

    const char *pfd1_str = getenv("PIPE_WRITE");
    if (pfd1_str) close(atoi(pfd1_str));

    if (dup2(pfd_read, STDIN_FILENO) < 0) { perror("[Logger] dup2"); exit(EXIT_FAILURE); }
    close(pfd_read);

    /* Open log file */
    FILE *logfp = fopen(LOG_FILE, "a");
    if (!logfp) {
        system("mkdir -p logs");
        logfp = fopen(LOG_FILE, "a");
        if (!logfp) { perror("[Logger] fopen"); exit(EXIT_FAILURE); }
    }

    /* Read alert lines from pipe */
    char line[ALERT_BUF];
    while (running && fgets(line, sizeof(line), stdin)) {
        size_t len = strlen(line);
        if (len > 0 && line[len - 1] == '\n') line[len - 1] = '\0';
        if (strlen(line) == 0) continue;

        if (strncmp(line, "SHUTDOWN", 8) == 0) {
            fprintf(logfp, "[SYSTEM] Graceful shutdown\n");
            fflush(logfp);
            break;
        }

        log_alert(logfp, line);
    }

    fclose(logfp);
    return 0;
}
