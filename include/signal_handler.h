#ifndef SIGNAL_HANDLER_H
#define SIGNAL_HANDLER_H

#include "common.h"

/* Child PIDs — defined in main.c, extern'd here */
extern pid_t sensor_pid;
extern pid_t aggregator_pid;
extern pid_t decision_pid;
extern pid_t logger_pid;

/* Pipe — defined in main.c, extern'd here */
extern int pipefd[2];

/* Public API */
void setup_signal_handlers(void);
void check_temperature(void);

/* Signal handler functions */
void handle_sigint(int sig);
void handle_sigusr1(int sig);
void handle_sigalrm(int sig);
void handle_sigusr2(int sig);

#endif /* SIGNAL_HANDLER_H */
