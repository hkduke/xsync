# Makefile for xsync
#
# @author: master@pepstack.com
#
# @create: 2018-10-15 17:22:00
# @update:
#######################################################################

XSYNC_PREFIX = .
XSYNC_SRC = $(XSYNC_PREFIX)/src


all:
	cd $(XSYNC_SRC) && $(MAKE) $@
	cd $(XSYNC_SRC)/kafkatools/ && $(MAKE) $@
	cd $(XSYNC_PREFIX)/target/ && ln -sf ../bin/watch-filters.lua watch-filters.lua
	cd $(XSYNC_PREFIX)/ && ln -sf watch-local watch

.PHONY: all