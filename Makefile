PIDGIN_PLUGIN_PATH=/home/mad/workspace/project/pidgin-dev/pidgin-2.5.5/pidgin/plugins/
PLUGIN_NAME=juick

all:
	cp $(PLUGIN_NAME).c $(PIDGIN_PLUGIN_PATH)
	$(MAKE) -C $(PIDGIN_PLUGIN_PATH) $(PLUGIN_NAME).so
	rm $(PIDGIN_PLUGIN_PATH)/$(PLUGIN_NAME).c
	mv $(PIDGIN_PLUGIN_PATH)/$(PLUGIN_NAME).so .
	strip $(PLUGIN_NAME).so
