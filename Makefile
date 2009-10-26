CFLAGS = -O2 -pipe -shared -fPIC -DPIC
PURPLE_CFLAGS = $(CFLAGS)
PURPLE_CFLAGS += $(shell pkg-config --cflags purple)
PURPLE_CFLAGS += $(shell pkg-config --cflags pidgin)

# for win
#PIDGIN_DIR=/home/mad/git/pidgin-clone/pidgin/plugins/
#GTK_TOP=/home/mad/workspace/project/pidgin-dev/win32-dev/gtk_2_0/
PIDGIN_DIR=../../pidgin-2.6.2/pidgin/plugins
GTK_TOP=../../win32-dev/gtk_2_0
#PIDGIN_DIR=../../workspace/pidgin-2.6.3/pidgin/plugins
#GTK_TOP=../../../../workspace/win32-dev/gtk_2_0

all:
	gcc -Wall ${PURPLE_CFLAGS} juick.c -o juick.so
	strip juick.so
	cp juick.so ~/.purple/plugins/omg.so

win:
	cp juick.c $(PIDGIN_DIR)
	$(MAKE) -f Makefile.mingw -C $(PIDGIN_DIR) juick.dll GTK_TOP=$(GTK_TOP)
	cp $(PIDGIN_DIR)/juick.dll .

	i486-mingw32-strip juick.dll
	cp juick.dll ~/.wine/drive_c/users/oleg/Application\ Data/.purple/plugins
	#strip juick.dll
	#cp juick.dll /cygdrive/c/Documents\ and\ Settings/user/Application\ Data/.purple/plugins/

test: test.c
	gcc test.c -o test ${PURPLE_CFLAGS} -lglib -lpurple
