TARGET := xsync-server

SOURCES := \
    server.c

SRC_DEFS := USE_MYSQL

SRC_INCDIRS := \
	. \
	.. \
	../common


# server has its own submakefile because it has a specific SRC_DEFS that
# we want to apply only to it
# SUBMAKEFILES := path/to/sub.mk

