#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <semaphore.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <errno.h>

/* ─── IPC Keys & Names ─────────────────────────────────────── */
#define SHM_KEY   0x1234
#define MSG_KEY   0x5678
#define SEM_NAME  "/car_sem"
#define FIFO_PATH "/tmp/car_monitor_fifo"
#define LOG_FILE  "logs/car_log.txt"

/* ─── Message Types ────────────────────────────────────────── */
#define MSG_TYPE_DATA     1
#define MSG_TYPE_WARNING  2
#define MSG_TYPE_CRITICAL 3

/* ─── Threshold Constants ───────────────────────────────────── */
/* Tyre Pressure (psi) */
#define PRESSURE_WARN_LOW      25
#define PRESSURE_WARN_HIGH     35
#define PRESSURE_CRIT_LOW      20
#define PRESSURE_CRIT_HIGH     40

/* Wheel Alignment (degrees) */
#define ALIGNMENT_WARN         40
#define ALIGNMENT_CRIT         45

/* Oil Level (%) */
#define OIL_WARN               25
#define OIL_CRIT               15

/* ─── Shared Memory Structure ───────────────────────────────── */
typedef struct {
    int alignment;    /* wheel alignment in degrees */
    int pressure;     /* tyre pressure in psi       */
    int oil_level;    /* oil level in percentage    */
} sensor_data;

/* ─── Message Queue Structure ───────────────────────────────── */
typedef struct {
    long type;
    int  alignment;
    int  pressure;
    int  oil_level;
    char text[256];   /* alert text for warnings/criticals */
} message;

/* ─── Pipe alert buffer size ────────────────────────────────── */
#define ALERT_BUF 512

#endif /* COMMON_H */
