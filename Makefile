#!/usr/bin/make -f
# Makefile for mod-system-plugins #
# ------------------------------- #
#

all:
	$(MAKE) -C mod-compressor
	$(MAKE) -C mod-compressor-advanced
	$(MAKE) -C mod-noisegate
	$(MAKE) -C mod-noisegate-advanced

install: all
	$(MAKE) install -C mod-compressor
	$(MAKE) install -C mod-compressor-advanced
	$(MAKE) install -C mod-noisegate
	$(MAKE) install -C mod-noisegate-advanced

clean:
	$(MAKE) clean -C mod-compressor
	$(MAKE) clean -C mod-compressor-advanced
	$(MAKE) clean -C mod-noisegate
	$(MAKE) clean -C mod-noisegate-advanced
