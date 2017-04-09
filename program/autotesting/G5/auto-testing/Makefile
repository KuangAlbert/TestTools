ifndef SDK_PATH
$(error source <GLSDK PATH>/linux-devkit/environment-setup")
endif

ifndef EXEC_DIR
$(error export EXEC_DIR")
endif

all: auto-testing

auto-testing: auto-testing.c auto-testing_ui.c drm_iotcl.c auto-testing_ui.h
	$(CC) ../../svauto-framework/sv-api/protocol.c auto-testing.c auto-testing_ui.c drm_iotcl.c fbutils.c font_12x22.c -o auto-testing -I${SDK_PATH}/${TARGET_SYS}/usr/include -I${GLSDK}/svauto-framework/sv-api -I ${SDK_PATH}/${TARGET_SYS}/usr/include/libdrm -I ${SDK_PATH}/${TARGET_SYS}/usr/include/omap

install:
	install -d $(EXEC_DIR)/usr/bin
	install auto-testing $(EXEC_DIR)/usr/bin

clean:
	rm -f auto-testing

