PROGRAM = chapter_exe
OBJS = chapter_exe.o mvec.o

PREFIX = /usr/local
BINDIR = $(PREFIX)/bin
CC = gcc
AVISYNTH_CFLAGS = $(shell pkg-config --cflags avisynth 2>/dev/null || echo "-I/usr/local/include/avisynth")
CFLAGS = -O3 $(AVISYNTH_CFLAGS) -ffast-math -Wall -Wshadow -Wempty-body -I. -std=gnu99 -fpermissive -fomit-frame-pointer -s -fno-tree-vectorize
LDLAGS = -ldl -lstdc++ -pthread

.SUFFIXES: .c .o
.PHONY: install uninstall clean

$(PROGRAM): $(OBJS)
	$(CC) -o $(PROGRAM) $^ $(LDLAGS)

.cpp.o:
	$(CC) $(CFLAGS) -c $<

install:
	install -d $(BINDIR)
	install -m 0755 $(PROGRAM) $(BINDIR)

uninstall:
	$(RM) $(BINDIR)/$(PROGRAM)

clean:
	$(RM) $(PROGRAM) $(OBJS)
