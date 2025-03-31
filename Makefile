# This is a personal academic project. Dear PVS-Studio, please check it.
# PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

GCCFLAGS = -Werror -Wall
CC = gcc 
LDFLAGS = -lrt -pthread

# Object files
OBJS = master.o player.o view.o shm_utils.o

# Agregar soporte para PVS-Studio
PVS_ANALYZER = pvs-studio-analyzer
PVS_REPORT = plog-converter

all: master all_players view

all_players: player_first_possible player_best_score player_random player_clock

master: master.o shm_utils.o
	$(CC) -o master master.o shm_utils.o $(LDFLAGS)

player_first_possible: player.o shm_utils.o
	$(CC) -o player_first_possible player.o shm_utils.o $(LDFLAGS)

player_best_score: player_best_score.o shm_utils.o
	$(CC) -o player_best_score player_best_score.o shm_utils.o $(LDFLAGS)

player_best_score.o: player.c shared_memory.h constants.h shm_utils.h
	$(CC) $(GCCFLAGS) -DBEST_SCORE -c player.c -o player_best_score.o

player.o: player.c shared_memory.h constants.h shm_utils.h
	$(CC) $(GCCFLAGS) -DFIRST_POSSIBLE -c player.c -o player.o

player_random: player_random.o shm_utils.o
	$(CC) -o player_random player_random.o shm_utils.o $(LDFLAGS)

player_random.o: player.c shared_memory.h constants.h shm_utils.h
	$(CC) $(GCCFLAGS) -DRANDOM -c player.c -o player_random.o

player_clock: player_clock.o shm_utils.o
	$(CC) -o player_clock player_clock.o shm_utils.o $(LDFLAGS)

player_clock.o: player.c shared_memory.h constants.h shm_utils.h
	$(CC) $(GCCFLAGS) -DCLOCK -c player.c -o player_clock.o

view: view.o shm_utils.o
	$(CC) -o view view.o shm_utils.o $(LDFLAGS)

%.o: %.c shared_memory.h constants.h shm_utils.h
	$(CC) $(GCCFLAGS) -c $<

clean:
	rm -rf *.o master player_first_possible player_random player_best_score player_clock view PVS-Studio.html *log strace_out

# Comandos para análisis
analyze: clean
	$(PVS_ANALYZER) trace -- make all
	$(PVS_REPORT) -a GA:1,2 -t tasklist -o PVS-Studio.log PVS-Studio.log

# Limpieza de archivos generados por PVS-Studio
clean-pvs:
	rm -f PVS-Studio.log