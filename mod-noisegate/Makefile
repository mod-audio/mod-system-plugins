#!/usr/bin/make -f
# Makefile for system-noisegate.lv2 #
# --------------------------------- #

include ../Makefile.mk

NAME = system-noisegate

# --------------------------------------------------------------
# Installation path

PREFIX ?= /usr/local
COMPLETE_INSTALL_PATH = $(DESTDIR)$(PREFIX)/lib/lv2/$(NAME).lv2

# --------------------------------------------------------------
# Default target is to build all plugins

all: build
build: $(NAME)-build

# --------------------------------------------------------------
# Build rules

$(NAME)-build: $(NAME).lv2/$(NAME)$(LIB_EXT)

$(NAME).lv2/$(NAME)$(LIB_EXT): $(NAME).c gate_core.c circular_buffer.c
	$(CC) $^ $(BUILD_C_FLAGS) $(LINK_FLAGS) -lm $(SHARED) -o $@

# --------------------------------------------------------------

clean:
	rm -f $(NAME).lv2/$(NAME)$(LIB_EXT)

# --------------------------------------------------------------

install: build
	install -d $(DESTDIR)$(PREFIX)/lib/lv2/$(NAME).lv2

	install -m 644 $(NAME).lv2/*.so  $(COMPLETE_INSTALL_PATH)/
	install -m 644 $(NAME).lv2/*.ttl $(COMPLETE_INSTALL_PATH)/
	cp -rv $(NAME).lv2/modgui $(COMPLETE_INSTALL_PATH)/

# --------------------------------------------------------------

