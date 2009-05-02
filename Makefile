PIDGIN_PLUGIN_PATH=/home/mad/pidgin-2.5.5/pidgin/plugins

all:
	cp juick.c $(PIDGIN_PLUGIN_PATH)
	$(MAKE) -C $(PIDGIN_PLUGIN_PATH) juick.so
	rm $(PIDGIN_PLUGIN_PATH)/juick.c
	mv $(PIDGIN_PLUGIN_PATH)/juick.so .
