#ifndef IPC_H
#define IPC_H

#include "common.h"

/* Shared Memory */
int  create_shared_memory(void);
int  open_shared_memory(void);
void cleanup_shared_memory(int shmid, sensor_data *data);

/* Message Queue */
int  create_message_queue(void);
int  open_message_queue(void);
void cleanup_message_queue(int msgid);

/* Named Semaphore */
sem_t *create_semaphore(void);
sem_t *open_semaphore_with_retry(int retries);
void   close_semaphore(sem_t *sem);
void   unlink_semaphore(void);

/* FIFO */
int  create_fifo(void);
void cleanup_fifo(void);

#endif /* IPC_H */
