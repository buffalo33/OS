CFLAGS += -Wall -Wextra -I .
SUFFIXE += -lm

ifdef debug
	CFLAGS += -g -D VERBOSE -Werror
else
	CFLAGS += -O3
endif

# Don't use preemption when compiling on the leaderboard
ifeq ($(findstring gitlab, $(shell pwd)),)
	deadlock = true
endif

ifdef nodeadlock
	undefine deadlock
endif

ifdef deadlock
	CFLAGS += -D DEADLOCK_DETECTION
endif

TEST_DIR  += test
BUILD_DIR += build
DEST_DIR  += install

THREAD_LIB = $(BUILD_DIR)/libthread.a

# FIXME: Remove [!7] when more functionalities are implemented
TEST_SOURCES = $(wildcard $(TEST_DIR)/[!7]*.c)
TEST_TARGETS = $(TEST_SOURCES:$(TEST_DIR)/%.c=$(BUILD_DIR)/%)
TEST_TARGETS_PTHREAD = $(addsuffix -pthread, $(TEST_TARGETS))


.PHONY: all check valgrind pthreads graphs install clean hooks

all: $(TEST_TARGETS)

$(BUILD_DIR)/libthread.o: thread.c thread.h utils.h thread_private.h
	gcc $(CFLAGS) -c -o $@ $<

$(THREAD_LIB): %.a: %.o
	ar rcs $@ $<

$(TEST_TARGETS): $(BUILD_DIR)/%: $(TEST_DIR)/%.c $(THREAD_LIB)
	gcc $(CFLAGS) $^ -o $@ $(SUFFIXE)

$(TEST_TARGETS_PTHREAD): $(BUILD_DIR)/%-pthread: $(TEST_DIR)/%.c thread.h
	gcc $(CFLAGS) -D USE_PTHREAD -pthread $^ -o $@ $(SUFFIXE)

pthreads: $(TEST_TARGETS_PTHREAD)
pthread: pthreads

check: clean-pthread all
	python3 -u script-test.py --quiet

valgrind: clean-pthread all
	python3 -u script-test.py --valgrind --quiet

graphs: pthreads all
	python3 -u make_graph.py

install: all $(TEST_TARGETS_PTHREAD)
	mkdir -p $(DEST_DIR)/lib
	cp $(THREAD_LIB) $(DEST_DIR)/lib

	mkdir -p $(DEST_DIR)/bin
	cp $(BUILD_DIR)/[[:digit:]]* $(DEST_DIR)/bin

hooks:
	cp --preserve --link hooks/* .git/hooks/

clean:
	rm -rf build/*
	rm -rf install/*

clean-pthread:
	rm -rf build/*-pthread
	rm -rf build/**-pthread
