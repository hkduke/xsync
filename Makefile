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


clean:
	cd $(XSYNC_SRC) && $(MAKE) $@
	cd $(XSYNC_SRC)/kafkatools/ && $(MAKE) $@
	rm -rf $(XSYNC_PREFIX)/target
	rm -rf $(XSYNC_PREFIX)/build
	rm -rf $(XSYNC_PREFIX)/dist

updpkg: clean
	$(XSYNC_PREFIX)/update-pkg.sh


dist:
	$(XSYNC_PREFIX)/update-pkg.sh -d


.PHONY: all clean updpkg dist
