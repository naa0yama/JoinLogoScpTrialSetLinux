PROGRAM = logoframe
OBJS = logoframe.o logoframe_det.o logoframe_mul.o

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
CC = gcc
AVISYNTH_CFLAGS = $(shell pkg-config --cflags avisynth 2>/dev/null || echo "-I/usr/local/include/avisynth")
CFLAGS = -O3 -xc++ $(AVISYNTH_CFLAGS) -ffast-math -Wall -Wshadow -fpermissive -Wempty-body -I. -fomit-frame-pointer -s -fno-tree-vectorize -fno-zero-initialized-in-bss
LDLAGS = -ldl -pthread -lstdc++

.SUFFIXES: .c .o
.PHONY: install uninstall clean

$(PROGRAM): $(OBJS)
	$(CC) -o $(PROGRAM) $^ $(LDLAGS)

.c.o:
	$(CC) $(CFLAGS) -c $<

install:
	install -d $(BINDIR)
	install -m 0755 $(PROGRAM) $(BINDIR)

uninstall:
	$(RM) $(BINDIR)/$(PROGRAM)

clean:
	$(RM) $(PROGRAM) $(OBJS)
