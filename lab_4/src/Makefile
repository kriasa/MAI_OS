CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -fPIC
LDFLAGS = -lm
DLFLAGS = -ldl

all: libmath1.so libmath2.so program1 program2

libmath1.so: libmath1.c mathlib.h
	$(CC) $(CFLAGS) -shared -o $@ libmath1.c $(LDFLAGS)

libmath2.so: libmath2.c mathlib.h
	$(CC) $(CFLAGS) -shared -o $@ libmath2.c $(LDFLAGS)

program1: program1.c libmath1.so
	$(CC) $(CFLAGS) -o $@ program1.c -L. -lmath1 $(LDFLAGS) -Wl,-rpath,.

program2: program2.c mathlib.h
	$(CC) $(CFLAGS) -o $@ program2.c $(DLFLAGS) $(LDFLAGS)

clean:
	rm -f libmath1.so libmath2.so program1 program2

test1: program1
	@echo "--- Запуск Программы 1 (Статическая) ---"
	@./program1

test2: program2 libmath1.so libmath2.so
	@echo "--- Запуск Программы 2 (Динамическая) ---"
	@./program2