CFLAGS = -O2 -pipe -shared -fPIC -DPIC
PURPLE_CFLAGS = $(CFLAGS) -DPURPLE_PLUGINS
PURPLE_CFLAGS += $(shell pkg-config --cflags purple)
PURPLE_CFLAGS += $(shell pkg-config --cflags pidgin)

# for win
PIDGIN_DIR=/home/mad/git/pidgin-clone/pidgin/plugins/
GTK_TOP=/home/mad/workspace/project/pidgin-dev/win32-dev/gtk_2_0/

all:
	gcc -Wall ${PURPLE_CFLAGS} juick.c juick-mood.c -o juick.so
	strip juick.so
	cp juick.so ~/.purple/plugins/omg.so

win:
	cp juick.c $(PIDGIN_DIR)
	$(MAKE) -f Makefile.mingw -C $(PIDGIN_DIR) juick.dll GTK_TOP=$(GTK_TOP)
	cp $(PIDGIN_DIR)/juick.dll .
	i586-mingw32msvc-strip juick.dll

test: test.c
	gcc test.c -o test ${PURPLE_CFLAGS} -lglib -lpurple
