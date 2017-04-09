#
# @file  Makefile
# @brief Makefile for auto-testing
# @copyright (C) 2016 by Desay SV automotive
#

TARGET := auto-testing

CFLAGS = -I${PKG_CONFIG_SYSROOT_DIR}/usr/include/libdrm \
	 -I${PKG_CONFIG_SYSROOT_DIR}/usr/include/drm \
	 -I${PKG_CONFIG_SYSROOT_DIR}/usr/include/omap

LDFLAGS = -ldrm -lm -lpthread -lMali

.PHONY: $(TARGET) clean

all: $(TARGET)

auto-testing:
	$(CC)  auto-testing.c fbutils.c font_12x22.c font_8x8.c -o $(OUTDIR)/auto-testing ${CFLAGS} ${LDFLAGS}


