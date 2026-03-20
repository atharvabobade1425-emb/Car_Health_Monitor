# Car_Health_Monitoring_system 

> A real-time car health monitoring system built in C using multi-process architecture, POSIX threads, and System V / POSIX IPC mechanisms ,Signals ,Process Control.

![Platform](https://img.shields.io/badge/platform-Linux-blue)
![Language](https://img.shields.io/badge/language-C-lightgrey)
![License](https://img.shields.io/badge/license-MIT-green)
![Build](https://img.shields.io/badge/build-make-orange)

---

## Overview

**carwatch** simulates a vehicle health monitoring system that continuously reads sensor data from three onboard sensors — tyre pressure, wheel alignment, and oil level — and raises real-time alerts when readings cross defined thresholds.

It is a complete Linux systems programming project demonstrating:

- Multi-process architecture with `fork()` + `execv()`
- POSIX threads (`pthreads`) for concurrent sensor simulation
- System V Shared Memory for sensor data exchange
- POSIX Named Semaphore as a mutex protecting shared memory
- System V Message Queue for inter-process communication
- Anonymous Pipe for alert delivery to the logger
- POSIX Signal Handling (`SIGINT`, `SIGUSR1`, `SIGALRM`, `SIGUSR2`)

---

# Architecture

<img width="1440" height="1736" alt="image" src="https://github.com/user-attachments/assets/59dbb611-d7e7-40f4-a4f7-5f471d759486" />


# Project Structure

```
carwatch/
├── src/
│   ├── main.c               # Supervisor process
│   ├── sensor_manager.c     # Sensor process with threads
│   ├── data_aggregator.c    # Reads shm, sends to decision engine
│   ├── decision_engine.c    # Evaluates thresholds, raises alerts
│   └── logger.c             # Receives alerts, writes to log file
├── threads/
│   ├── pressure_thread.c    # Tyre pressure sensor thread
│   ├── alignment_thread.c   # Wheel alignment sensor thread
│   └── oil_level_thread.c   # Oil level sensor thread
├── signals/
│   └── signal_handler.c     # SIGINT, SIGUSR1, SIGALRM, SIGUSR2
├── include/
│   ├── common.h             # Shared structs, keys, thresholds
│   ├── threads.h            # Thread function declarations
│   ├── ipc.h                # IPC function declarations
│   └── signal_handler.h     # Signal handler declarations
├── logs/
│   └── car_log.txt          # Generated at runtime
├── build/                   # Compiled binaries
└── Makefile
```

---

# IPC Mechanisms Used

| Mechanism | Key / Name | Purpose |
|---|---|---|
| System V Shared Memory | `SHM_KEY 0x1234` | Sensor readings shared between threads and aggregator |
| POSIX Named Semaphore | `/car_sem` | Mutex protecting shared memory access |
| System V Message Queue | `MSG_KEY 0x5678` | Sensor snapshots from aggregator to decision engine |
| Anonymous Pipe | `pipefd[0/1]` | Alert strings from decision engine to logger |
| Named FIFO | `/tmp/car_monitor_fifo` | Shutdown notification channel |

---

## Sensor Thresholds

### Tyre Pressure (psi)
| Level | Condition |
|---|---|
| Normal | 25 – 35 psi |
| WARNING | < 25 or > 35 psi |
| CRITICAL | < 20 or > 40 psi |

### Wheel Alignment (degrees)
| Level | Condition |
|---|---|
| Normal | < 40 degrees |
| WARNING | > 40 degrees |
| CRITICAL | > 45 degrees |

### Oil Level (%)
| Level | Condition |
|---|---|
| Normal | > 25% |
| WARNING | < 25% |
| CRITICAL | < 15% |

---

## Processes

| Binary | Role |
|---|---|
| `supervisor` | Parent process. Forks all children, owns signal handlers, runs health check timer every 10s |
| `sensor_manager` | Creates shared memory + semaphore. Spawns 3 sensor threads |
| `data_aggregator` | Reads shared memory every 5s, sends snapshot via message queue |
| `decision_engine` | Evaluates thresholds, writes alerts to pipe, sends `SIGUSR2` on critical |
| `logger` | Reads alerts from pipe, prints to terminal, appends to `logs/car_log.txt` |

---

## Signal Handling

| Signal | Trigger | Action |
|---|---|---|
| `SIGINT` | `Ctrl+C` | Graceful shutdown — kills all children, cleans IPC |
| `SIGUSR1` | `kill -USR1 <pid>` | Prints snapshot of all child PIDs |
| `SIGALRM` | Auto every 10s | Health check + temperature check |
| `SIGUSR2` | Critical sensor / temp > 30°C | Prints critical alert to terminal |

---

# Requirements

- Linux (Ubuntu 20.04+ recommended)
- GCC with pthread support
- POSIX-compliant system (`sem_open`, `shm`, `msg`)

```bash
sudo apt update
sudo apt install gcc make
```

---

# Build & Run

```bash
# Clone the repo
git clone https://github.com/yourusername/carwatch.git
cd carwatch

# Compile all 5 binaries
make

# Run the system
make run

# Clean all build artifacts and IPC resources
make clean
```

---

# Expected Output

```
Smart Car Service Monitoring System started (pid=15717)
────────────────────────────────────────────────
Press Ctrl+C to stop. Snapshot every 5s.
────────────────────────────────────────────────

[Aggregator] ── Sensor Snapshot ──────────────
  Wheel Alignment : 34 degrees
  Tyre Pressure   : 28 psi
  Oil Level       : 67%
────────────────────────────────────────────────

[Aggregator] ── Sensor Snapshot ──────────────
  Wheel Alignment : 47 degrees
  Tyre Pressure   : 18 psi
  Oil Level       : 22%
────────────────────────────────────────────────
*** CRITICAL ALERT: Tyre pressure dangerously LOW (18 psi) — stop vehicle immediately! ***
*** CRITICAL ALERT: Wheel alignment critical (47 deg) — unsafe to drive! ***
*** CRITICAL ALERT: Oil level dangerously low (22%) — engine damage risk! ***
*** CRITICAL SENSOR ALERT — check log for details ***

[Health Check] System running normally

^C
[System] Shutting down...
[System] All processes terminated. Goodbye.
```

---

# Live Log

All alerts are timestamped and written to `logs/car_log.txt`.

```bash
tail -f logs/car_log.txt
```

```
[2024-11-15 14:32:10] *** CRITICAL ALERT: Tyre pressure dangerously LOW (18 psi) ***
[2024-11-15 14:32:10] *** CRITICAL ALERT: Wheel alignment critical (47 deg) ***
[2024-11-15 14:32:15]   WARNING: Oil level low (23%) — top up soon.
```

---

# Useful Commands

```bash
# Print snapshot of all process PIDs
kill -USR1 $(pgrep supervisor)

# Manually trigger overheating alert
kill -USR2 $(pgrep supervisor)

# Watch live log output
tail -f logs/car_log.txt

# Clean up stale IPC resources manually
ipcrm -M 0x1234
ipcrm -Q 0x5678
```

---

# Key Implementation Rules

1. `setup_signal_handlers()` is called **once** in the supervisor **before** any `fork()`
2. Every child binary starts with `alarm(0); signal(SIGALRM, SIG_DFL);` to cancel the inherited alarm
3. Semaphore is held for **minimum time only** — lock → copy → unlock → then `msgsnd`/`printf`
4. Aggregator uses a **10-retry loop** to open the semaphore (sensor_manager may not be ready yet)
5. Each thread uses a **unique random seed**: `srand(time(NULL) ^ pthread_self())`
6. Supervisor closes **both** pipe ends after all forks
7. All signals registered using `sigaction()`, not `signal()`

---

# Author

**Atharva Bobade**
B.Tech Electronics & Telecommunication Engineering
Embedded Systems & Linux Programming

---

# License

MIT License — free to use, modify, and distribute.
