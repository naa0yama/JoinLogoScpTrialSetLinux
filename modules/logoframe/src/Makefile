PROGRAM = logoframe
OBJS = logoframe.o logoframe_det.o logoframe_mul.o

CC = gcc
CFLAGS = -O3 -xc++ -I/usr/local/include/avisynth -ffast-math -Wall -Wshadow -fpermissive -Wempty-body -I. -fomit-frame-pointer -s -fno-tree-vectorize -fno-zero-initialized-in-bss
LDLAGS = -ldl -pthread -lstdc++

.SUFFIXES: .c .o

$(PROGRAM): $(OBJS)
	$(CC) -o $(PROGRAM) $^ $(LDLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	$(RM) $(PROGRAM) $(OBJS)
