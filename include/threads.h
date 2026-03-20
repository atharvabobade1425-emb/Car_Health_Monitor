#ifndef THREADS_H
#define THREADS_H

#include "common.h"
#include <pthread.h>

/* Thread entry-point functions */
void *pressure_thread(void *arg);
void *alignment_thread(void *arg);
void *oil_level_thread(void *arg);

#endif /* THREADS_H */
