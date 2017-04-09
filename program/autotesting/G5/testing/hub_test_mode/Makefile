ifndef SDK_PATH
$(error source <GLSDK PATH>/env_setup.sh")
endif

INCPATH = -I $(GLSDK)/linux-devkit/arm-linux-gnueabihf/usr/include/libusb-1.0
LIBSPATH = -L $(GLSDK)/linux-devkit/arm-linux-gnueabihf/lib/
LIBS = $(LIBSPATH) -lusb-1.0

.PHONY: hub_test_mode hub_dev_test_mode hub_full_test_mode

all: hub_test_mode hub_dev_test_mode hub_full_test_mode

hub_test_mode:
	$(CC) hub_test_mode.c -o hub_test_mode $(INCPATH) $(LIBS)

hub_dev_test_mode:
	$(CC) hub_dev_test_mode.c -o hub_dev_test_mode $(INCPATH) $(LIBS)

hub_full_test_mode:
	$(CC) hub_full_test_mode.c -o hub_full_test_mode $(INCPATH) $(LIBS)

install:
	install hub_test_mode $(EXEC_DIR)/usr/bin
	install hub_dev_test_mode $(EXEC_DIR)/usr/bin
	install hub_full_test_mode $(EXEC_DIR)/usr/bin

clean:
	rm -rf hub_test_mode hub_dev_test_mode hub_full_test_mode
