TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

SOURCES += \
        audio_player_board.c \
        audio_utils.c \
        io_utils.c \
        snd_player.c

HEADERS += \
    audio_player_board.h \
    audio_utils.h \
    io_utils.h

INCLUDEPATH += libxenomai/usr/xenomai/include


DESTDIR=$$PWD/libxenomai
XENOCONFIG=$$DESTDIR/usr/xenomai/bin/xeno-config

QMAKE_CC=	$(shell DESTDIR=$$DESTDIR $$XENOCONFIG --cc)
QMAKE_LINK = $(shell DESTDIR=$$DESTDIR $$XENOCONFIG --cc)
QMAKE_CFLAGS= -std=c99 $(shell DESTDIR=$$DESTDIR $$XENOCONFIG --posix --alchemy --cflags)
QMAKE_LFLAGS=$(shell DESTDIR=$$DESTDIR $$XENOCONFIG --posix --alchemy --ldflags --auto-init-solib)
