GCCFLAGS = -Werror -Wall -D_POSIX_C_SOURCE=200809L
CC = gcc 
LDFLAGS = -lrt -pthread

# Object files
OBJS = master.o player.o view.o shm_utils.o

all: master player view

master: master.o shm_utils.o
	$(CC) -o master master.o shm_utils.o $(LDFLAGS)

player: player.o shm_utils.o
	$(CC) -o player player.o shm_utils.o $(LDFLAGS)

view: view.o shm_utils.o
	$(CC) -o view view.o shm_utils.o $(LDFLAGS)

%.o: %.c shared_memory.h constants.h shm_utils.h
	$(CC) $(GCCFLAGS) -c $<

clean:
	rm -f *.o master player view