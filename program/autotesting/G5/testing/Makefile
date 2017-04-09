ifndef SDK_PATH
$(error source <GLSDK PATH>/linux-devkit/environment-setup")
endif

ifndef EXEC_DIR
$(error export EXEC_DIR")
endif

CROSS_COMPILE := arm-linux-gnueabihf-
CC := $(CROSS_COMPILE)gcc
AS := $(CROSS_COMPILE)as
LD := $(CROSS_COMPILE)ld
FLAG := -L${SDK_PATH}/${TARGET_SYS}/usr/lib

.PHONY: hub_test_mode

all: hub_test_mode testing-nor testing-mmc testing-ipod testing-ddr

hub_test_mode:
	$(MAKE) -C hub_test_mode

testing-nor: testing-nor.c
	$(CC) testing-nor.c -o testing-nor $(FLAG)

testing-mmc: testing-mmc.c
	$(CC) testing-mmc.c -o testing-mmc $(FLAG)

testing-ipod: testing-ipod.c
	$(CC) testing-ipod.c -o testing-ipod $(FLAG)

testing-ddr: testing-ddr.c
	$(CC) testing-ddr.c -o testing-ddr $(FLAG)

install:
	install testing-nor $(EXEC_DIR)/usr/bin
	install testing-mmc $(EXEC_DIR)/usr/bin
	install testing-ipod $(EXEC_DIR)/usr/bin
	install testing-ddr $(EXEC_DIR)/usr/bin
	$(MAKE) -C hub_test_mode install

clean:
	rm -f testing-nor testing-mmc testing-ipod testing-ddr
	$(MAKE) -C hub_test_mode clean
