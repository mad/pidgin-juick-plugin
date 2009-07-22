CFLAGS = -O2 -pipe -shared -fPIC -DPIC
PURPLE_CFLAGS = $(CFLAGS) -DPURPLE_PLUGINS
PURPLE_CFLAGS += $(shell pkg-config --cflags purple)
PURPLE_CFLAGS += $(shell pkg-config --cflags pidgin)

all:
	gcc -Wall ${PURPLE_CFLAGS} juick.c -o juick.so
	strip juick.so

test: test.c
	gcc test.c -o test \
	-I/usr/local/include/libpurple \
	-I/usr/include/glib-2.0 \
	-I/usr/lib/glib-2.0/include \
	-I/usr/local/include/pidgin \
	-I/usr/local/include/libpurple \
	-I/usr/include/gtk-2.0 \
	-I/usr/lib/gtk-2.0/include \
	-I/usr/include/atk-1.0 \
	-I/usr/include/cairo \
	-I/usr/include/pango-1.0 \
	-I/usr/include/glib-2.0 \
	-I/usr/lib/glib-2.0/include \
	-I/usr/include/pixman-1 \
	-I/usr/include/freetype2 \
	-I/usr/include/libpng12  -lglib -lpurple