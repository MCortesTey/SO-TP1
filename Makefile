# This is a personal academic project. Dear PVS-Studio, please check it.
# PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

GCCFLAGS = -lm -Werror -Wall -g -fsanitize=address -Wextra
CC = gcc 
LDFLAGS = -lm -lrt -pthread -fsanitize=address -g

# Object files
OBJS = master.o player.o view.o shm_utils.o

# Agregar soporte para PVS-Studio
PVS_ANALYZER = pvs-studio-analyzer
PVS_REPORT = plog-converter

all: master all_players view

all_players: player_first_possible player_best_score player_random player_clock

master: master.o shm_ADT.o # shm_utils.o
	$(CC) master.o shm_ADT.o $(LDFLAGS) -o master

player_first_possible: player.o shm_ADT.o # shm_utils.o
	$(CC) -o player_first_possible player.o shm_ADT.o $(LDFLAGS)

player_best_score: player_best_score.o shm_ADT.o # shm_utils.o
	$(CC) -o player_best_score player_best_score.o shm_ADT.o $(LDFLAGS)

player_best_score.o: player.c constants.h shm_ADT.h # shm_utils.h shared_memory.h
	$(CC) $(GCCFLAGS) -DBEST_SCORE -c player.c -o player_best_score.o

player.o: player.c constants.h shm_ADT.h # shm_utils.h shared_memory.h
	$(CC) $(GCCFLAGS) -DFIRST_POSSIBLE -c player.c -o player.o

player_random: player_random.o shm_ADT.o # shm_utils.o
	$(CC) -o player_random player_random.o shm_ADT.o $(LDFLAGS)

player_random.o: player.c constants.h shm_ADT.h # shm_utils.h shared_memory.h
	$(CC) $(GCCFLAGS) -DRANDOM -c player.c -o player_random.o

player_clock: player_clock.o shm_ADT.o # shm_utils.o
	$(CC) -o player_clock player_clock.o shm_ADT.o $(LDFLAGS)

player_clock.o: player.c constants.h shm_ADT.h # shm_utils.h shared_memory.h 
	$(CC) $(GCCFLAGS) -DCLOCK -c player.c -o player_clock.o

view: view.o shm_ADT.o # shm_utils.o
	$(CC) -o view view.o shm_ADT.o $(LDFLAGS)

%.o: %.c constants.h shm_ADT.h # shm_utils.h shared_memory.h
	$(CC) $(GCCFLAGS) -c $<

clean:
	rm -rf *.o master player_first_possible player_random player_best_score player_clock view PVS-Studio.html *log strace_out


# Comandos para análisis
analyze: clean
	$(PVS_ANALYZER) trace -- make all
	$(PVS_ANALYZER) analyze -o PVS-Studio.log
	$(PVS_REPORT) -a GA:1,2 -t tasklist -o PVS-Studio.log PVS-Studio.log

# Limpieza de archivos generados por PVS-Studio
clean-pvs:
	rm -f PVS-Studio.log