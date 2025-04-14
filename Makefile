# This is a personal academic project. Dear PVS-Studio, please check it.
# PVS-Studio Static Code Analyzer for C, C++, C#, and Java: https://pvs-studio.com

CC = gcc
GCCFLAGS = -lm -Wall -Werror -g -Wextra -fsanitize=address -Iinclude
LDFLAGS = -lm -lrt -pthread -g -fsanitize=address

PVS_ANALYZER = pvs-studio-analyzer
PVS_REPORT = plog-converter

# Source directories
SRC_DIR = src
CORE_DIR = $(SRC_DIR)/core
PROCESS_DIR = $(SRC_DIR)/process
UTILS_DIR = $(SRC_DIR)/utils

# Object files with updated paths
COMMON_OBJS = $(UTILS_DIR)/shared_memory.o $(UTILS_DIR)/constants.o $(UTILS_DIR)/sems.o 
PLAYER_OBJS = $(CORE_DIR)/player.o $(PROCESS_DIR)/player_best_score.o $(PROCESS_DIR)/player_random.o \
              $(PROCESS_DIR)/player_clock.o $(PROCESS_DIR)/player_killer.o $(PROCESS_DIR)/player_first_possible.o
MASTER_OBJS = $(CORE_DIR)/master.o $(PROCESS_DIR)/child_manager.o
VIEW_OBJS = $(CORE_DIR)/view.o

STRATEGIES = first_possible best_score random clock killer jason error
all: master all_players view

master: $(MASTER_OBJS) $(COMMON_OBJS) $(UTILS_DIR)/game.o
	$(CC) $(MASTER_OBJS) $(COMMON_OBJS) $(LDFLAGS) $(UTILS_DIR)/game.o -o master

all_players: $(addprefix player_,$(STRATEGIES))

player_%: $(PROCESS_DIR)/player_%.o $(COMMON_OBJS) $(UTILS_DIR)/players_strategies_%.o
	$(CC) $(LDFLAGS) $^ -o $@

$(PROCESS_DIR)/player_%.o: $(CORE_DIR)/player.c include/constants.h include/shared_memory.h include/players_strategies.h
	$(CC) $(GCCFLAGS) -D$(shell echo $* | tr '[:lower:]' '[:upper:]') -c $< -o $@

view: $(VIEW_OBJS) $(COMMON_OBJS)
	$(CC) -o view $(VIEW_OBJS) $(COMMON_OBJS) $(LDFLAGS)

$(UTILS_DIR)/%.o: $(UTILS_DIR)/%.c include/constants.h include/macros.h include/*
	$(CC) $(GCCFLAGS) -c $< -o $@

$(PROCESS_DIR)/child_manager.o: $(PROCESS_DIR)/child_manager.c include/constants.h include/shared_memory.h
	$(CC) $(GCCFLAGS) -c $< -o $@

$(CORE_DIR)/%.o: $(CORE_DIR)/%.c
	$(CC) $(GCCFLAGS) -c $< -o $@

$(UTILS_DIR)/players_strategies_%.o: $(UTILS_DIR)/players_strategies.c include/constants.h include/players_strategies.h 
	$(CC) $(GCCFLAGS) -D$(shell echo $* | tr '[:lower:]' '[:upper:]') -c $< -o $@

clean:
	rm -rf $(SRC_DIR)/**/*.o ChompChamps player_* view PVS-Studio.html *log strace_out

clean-pvs:
	rm -f PVS-Studio.log

analyze: clean
	$(PVS_ANALYZER) trace -- make all
	$(PVS_ANALYZER) analyze -o PVS-Studio.log
	$(PVS_REPORT) -a GA:1,2 -t fullhtml PVS-Studio.log -o PVS-Studio.html

CFLAGS = $(GCCFLAGS)

.PHONY: all clean analyze clean-pvs