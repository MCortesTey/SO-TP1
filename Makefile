# This is a personal academic project. Dear PVS-Studio, please check it.
# PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

CC = gcc
GCCFLAGS = -lm -Werror -Wall -g -Wextra -fsanitize=address
LDFLAGS = -lm -lrt -pthread -g -fsanitize=address

PVS_ANALYZER = pvs-studio-analyzer
PVS_REPORT = plog-converter

COMMON_OBJS = shm_ADT.o constants.o sems.o
PLAYER_OBJS = player.o player_best_score.o player_random.o player_clock.o player_killer.o player_first_possible.o
MASTER_OBJS = master.o child_manager.o
VIEW_OBJS = view.o

all: master all_players view

master: $(MASTER_OBJS) $(COMMON_OBJS) game.o
	$(CC) $(MASTER_OBJS) $(COMMON_OBJS) $(LDFLAGS) game.o -o ChompChamps

all_players: player_first_possible player_best_score player_random player_clock player_killer player_jason

player_first_possible: player_first_possible.o $(COMMON_OBJS)
	$(CC) $(LDFLAGS) player_first_possible.o $(COMMON_OBJS) -o player_first_possible

player_best_score: player_best_score.o $(COMMON_OBJS)
	$(CC) $(LDFLAGS) player_best_score.o $(COMMON_OBJS) -o player_best_score

player_random: player_random.o $(COMMON_OBJS)
	$(CC) $(LDFLAGS) player_random.o $(COMMON_OBJS) -o player_random

player_clock: player_clock.o $(COMMON_OBJS)
	$(CC) $(LDFLAGS) player_clock.o $(COMMON_OBJS) -o player_clock

player_killer: player_killer.o $(COMMON_OBJS)
	$(CC) $(LDFLAGS) player_killer.o $(COMMON_OBJS) -o player_killer

player_jason: player_jason.o $(COMMON_OBJS)
	$(CC) $(LDFLAGS) player_jason.o $(COMMON_OBJS) -o player_jason

player_first_possible.o: player.c constants.h shm_ADT.h
	$(CC) $(GCCFLAGS) -DFIRST_POSSIBLE -c player.c -o player_first_possible.o

player_best_score.o: player.c constants.h shm_ADT.h
	$(CC) $(GCCFLAGS) -DBEST_SCORE -c player.c -o player_best_score.o

player_random.o: player.c constants.h shm_ADT.h
	$(CC) $(GCCFLAGS) -DRANDOM -c player.c -o player_random.o

player_clock.o: player.c constants.h shm_ADT.h
	$(CC) $(GCCFLAGS) -DCLOCK -c player.c -o player_clock.o

player_killer.o: player.c constants.h shm_ADT.h
	$(CC) $(GCCFLAGS) -DKILLER -c player.c -o player_killer.o

player_jason.o: player.c constants.h shm_ADT.h
	$(CC) $(GCCFLAGS) -DJASON -c player.c -o player_jason.o

view: $(VIEW_OBJS) $(COMMON_OBJS)
	$(CC) -o view $(VIEW_OBJS) $(COMMON_OBJS) $(LDFLAGS)

%.o: %.c constants.h macros.h
	$(CC) $(GCCFLAGS) -c $<

child_manager.o: child_manager.c constants.h shm_ADT.h
	$(CC) $(GCCFLAGS) -c child_manager.c -o child_manager.o

clean:
	rm -rf *.o ChompChamps player_* view PVS-Studio.html *log strace_out

clean-pvs:
	rm -f PVS-Studio.log

analyze: clean
	$(PVS_ANALYZER) trace -- make all
	$(PVS_ANALYZER) analyze -o PVS-Studio.log
	$(PVS_REPORT) -a GA:1,2 -t fullhtml PVS-Studio.log -o PVS-Studio.html

.PHONY: all clean analyze clean-pvs