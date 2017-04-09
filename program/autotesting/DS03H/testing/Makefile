#
# @file  Makefile
# @brief Makefile for auto-testing
# @copyright (C) 2016 by Desay SV automotive
#

TARGET := testing-ipod testing-nor testing-ddr testing-mmc testing-lvds testing-decoder

.PHONY: $(TARGET) clean

all: $(TARGET)

testing-ipod:
	$(CC) testing-ipod.c -o $(OUTDIR)/testing-ipod

testing-nor:
	$(CC) testing-nor.c -o $(OUTDIR)/testing-nor

testing-ddr:
	$(CC) testing-ddr.c -o $(OUTDIR)/testing-ddr

testing-mmc:
	$(CC) testing-mmc.c -o $(OUTDIR)/testing-mmc

testing-lvds:
	$(CC) testing-lvds.c -o $(OUTDIR)/testing-lvds

testing-decoder:
	$(CC) testing-decoder.c -o $(OUTDIR)/testing-decoder

