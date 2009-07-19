CFLAGS = -O2 -pipe -shared -fPIC -DPIC
PURPLE_CFLAGS = $(CFLAGS) -DPURPLE_PLUGINS
PURPLE_CFLAGS += $(shell pkg-config --cflags purple)
PURPLE_CFLAGS += $(shell pkg-config --cflags pidgin)

all:
	gcc -Wall ${PURPLE_CFLAGS} juick.c -o juick.so
	strip juick.so
