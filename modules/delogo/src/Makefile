PROGRAM = libdelogo.so
OBJS = delogo.o

PREFIX = /usr/local/lib/avisynth
RM = rm
CC = gcc
AVISYNTH_CFLAGS = $(shell pkg-config --cflags avisynth 2>/dev/null || echo "-I/usr/local/include/avisynth")
CFLAGS = -O3 $(AVISYNTH_CFLAGS) -fPIC -ffast-math -Wall -Wshadow -Wempty-body -I. -std=gnu99 -fpermissive -fomit-frame-pointer -s -fno-tree-vectorize
LDLAGS = -shared -fPIC

.SUFFIXES: .c .o
.PHONY: install uninstall clean

$(PROGRAM): $(OBJS)
	$(CC) -o $(PROGRAM) $^ $(LDLAGS)

.cpp.o:
	$(CC) $(CFLAGS) -c $<

clean:
	$(RM) $(PROGRAM) $(OBJS)

install:
	install -d $(PREFIX)
	install -m 0755 $(PROGRAM) $(PREFIX)

uninstall:
	$(RM) $(PREFIX)$(PROGRAM)
