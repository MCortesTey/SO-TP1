GCCFLAGS = -Werror -Wall
CC = gcc 
LDFLAGS = -lrt -pthread

# Object files
OBJS = master.o player.o view.o shm_utils.o

all: master player_first_possible player_sparse view

all_players: player_first_possible player_best_score player_random

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

view: view.o shm_utils.o
	$(CC) -o view view.o shm_utils.o $(LDFLAGS)

%.o: %.c shared_memory.h constants.h shm_utils.h
	$(CC) $(GCCFLAGS) -c $<

clean:
	rm -f *.o master player_first_possible player_random player_best_score view