DESTDIR=libxenomai
XENOCONFIG=$(DESTDIR)/usr/xenomai/bin/xeno-config
CC=	$(shell DESTDIR=$(DESTDIR) $(XENOCONFIG) --cc)
CFLAGS= -std=c99 $(shell DESTDIR=$(DESTDIR) $(XENOCONFIG) --posix --alchemy --cflags)
LDFLAGS=$(shell DESTDIR=$(DESTDIR) $(XENOCONFIG) --posix --alchemy --ldflags --auto-init-solib)

all: snd_player

snd_player: snd_player.o io_utils.o audio_player_board.o audio_utils.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%.o : %.c
	$(CC) -o $@ -c $< $(CFLAGS) -Wall

clean:
	rm -f *.o snd_player

.PHONY: clean
