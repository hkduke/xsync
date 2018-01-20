TARGET := xsync-server

LIB_PREFIX := ${TARGET_DIR}/../libs/lib

TGT_LDFLAGS := -L${TARGET_DIR}

TGT_LDLIBS  := \
	${LIB_PREFIX}/libcommon.a \
	${LIB_PREFIX}/liblog4c.a

SOURCES := \
    server.c

SRC_DEFS := DEBUG

SRC_INCDIRS := \
    . \
	.. \
	../common
