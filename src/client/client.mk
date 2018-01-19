TARGET := xsync-client

SOURCES := \
    client.c

SRC_DEFS := USE_MYSQL

SRC_INCDIRS := \
	. \
	.. \
	../common


# client has its own submakefile because it has a specific SRC_DEFS that
# we want to apply only to it
# SUBMAKEFILES := path/to/sub.mk
