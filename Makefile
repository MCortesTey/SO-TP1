GCCFLAGS = -Werror -Wall -std=c99
CC = gcc 

all: master player view shm

master: master.o
	$(CC) -o master master.o

player: player.o
	$(CC) -o player player.o

view: view.o
	$(CC) -o view view.o

shm: shm.o
	$(CC) -o shm shm.o master.o player.o view.o

%.o: %.c
	$(CC) $(GCCFLAGS) -c $<

clean:
	rm -f *.o master player view shm