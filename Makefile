# ═══════════════════════════════════════════════════════════════
# Makefile — Smart Car Service Monitoring System
# ═══════════════════════════════════════════════════════════════

CC      = gcc
CFLAGS  = -Wall -Wextra -pthread -I include
LDFLAGS = -pthread -lrt

# ── Directories ─────────────────────────────────────────────────
SRC     = src
THR     = threads
SIG     = signals
BLD     = build

# ── All targets ─────────────────────────────────────────────────
ALL_BINS = $(BLD)/supervisor      \
           $(BLD)/sensor_manager  \
           $(BLD)/data_aggregator \
           $(BLD)/decision_engine \
           $(BLD)/logger

.PHONY: all run clean

all: $(BLD) logs $(ALL_BINS)

# ── Create output dirs if missing ───────────────────────────────
$(BLD):
	mkdir -p $(BLD)

logs:
	mkdir -p logs

# ════════════════════════════════════════════════════════════════
# Object files
# ════════════════════════════════════════════════════════════════

# Signal handler (linked into supervisor only)
$(BLD)/signal_handler.o: $(SIG)/signal_handler.c include/signal_handler.h include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

# Sensor threads (linked into sensor_manager only)
$(BLD)/pressure_thread.o: $(THR)/pressure_thread.c include/threads.h include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BLD)/alignment_thread.o: $(THR)/alignment_thread.c include/threads.h include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BLD)/oil_level_thread.o: $(THR)/oil_level_thread.c include/threads.h include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

# Main source objects
$(BLD)/main.o: $(SRC)/main.c include/common.h include/signal_handler.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BLD)/sensor_manager.o: $(SRC)/sensor_manager.c include/common.h include/threads.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BLD)/data_aggregator.o: $(SRC)/data_aggregator.c include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BLD)/decision_engine.o: $(SRC)/decision_engine.c include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

$(BLD)/logger.o: $(SRC)/logger.c include/common.h
	$(CC) $(CFLAGS) -c $< -o $@

# ════════════════════════════════════════════════════════════════
# Binary targets
# ════════════════════════════════════════════════════════════════

# supervisor = main.o + signal_handler.o
$(BLD)/supervisor: $(BLD)/main.o $(BLD)/signal_handler.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "[Makefile] Built: $@"

# sensor_manager = sensor_manager.o + 3 thread objects
$(BLD)/sensor_manager: $(BLD)/sensor_manager.o \
                        $(BLD)/pressure_thread.o \
                        $(BLD)/alignment_thread.o \
                        $(BLD)/oil_level_thread.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "[Makefile] Built: $@"

# data_aggregator
$(BLD)/data_aggregator: $(BLD)/data_aggregator.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "[Makefile] Built: $@"

# decision_engine
$(BLD)/decision_engine: $(BLD)/decision_engine.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "[Makefile] Built: $@"

# logger
$(BLD)/logger: $(BLD)/logger.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)
	@echo "[Makefile] Built: $@"

# ════════════════════════════════════════════════════════════════
# Run
# ════════════════════════════════════════════════════════════════
run: all
	@echo "════════════════════════════════════════════"
	@echo " Starting Smart Car Service Monitoring System"
	@echo " Press Ctrl+C to stop"
	@echo "════════════════════════════════════════════"
	./$(BLD)/supervisor

# ════════════════════════════════════════════════════════════════
# Clean
# ════════════════════════════════════════════════════════════════
clean:
	@echo "[Makefile] Cleaning build artifacts..."
	rm -f $(BLD)/*.o $(ALL_BINS)
	rm -f $(BLD)/supervisor $(BLD)/sensor_manager \
	      $(BLD)/data_aggregator $(BLD)/decision_engine $(BLD)/logger
	rm -f logs/car_log.txt
	rm -f $(FIFO_PATH) /tmp/car_monitor_fifo
	@echo "[Makefile] Removing stale IPC resources (errors are safe to ignore)..."
	-ipcrm -M 0x1234 2>/dev/null || true
	-ipcrm -Q 0x5678 2>/dev/null || true
	-sem_unlink /car_sem 2>/dev/null || true
	-rm -f /tmp/car_monitor_fifo 2>/dev/null || true
	@echo "[Makefile] Clean complete."
