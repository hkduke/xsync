TARGET := xsync-client

TGT_LDFLAGS := -L${TARGET_DIR}
TGT_LDLIBS  := -lcommon
TGT_PREREQS := libcommon.a

SOURCES := \
    client.c

SRC_DEFS := DEBUG

SRC_INCDIRS := \
	.. \
	../common

# xsync-client has its own submakefile because it has a dependency on
# libcommon.a
SUBMAKEFILES := ../common/common.mk
