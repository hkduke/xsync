TARGET := xsync-server

LIB_PREFIX := ${TARGET_DIR}/../libs/lib

TGT_LDFLAGS := -L${TARGET_DIR}

TGT_LDLIBS  := -lcommon \
	${LIB_PREFIX}/liblog4c.a

TGT_PREREQS := libcommon.a


SOURCES := \
    server.c

SRC_DEFS := DEBUG

SRC_INCDIRS := \
	.. \
	../common

# xsync-server has its own submakefile because it has a dependency on
# libcommon.a
SUBMAKEFILES := ../common/common.mk
