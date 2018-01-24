# see "client.mk.another" for another style of client.mk
#
TARGET := xsync-client

LIB_PREFIX := ${TARGET_DIR}/../libs/lib

TGT_LDFLAGS := -L${TARGET_DIR}

TGT_LDLIBS  := \
	${LIB_PREFIX}/libcommon.a \
	${LIB_PREFIX}/liblog4c.a \
	${LIB_PREFIX}/libexpat.a \
	${LIB_PREFIX}/libsqlite3.a \
	${LIB_PREFIX}/libjemalloc.a \
	${LIB_PREFIX}/libmxml.a \
	${LIB_PREFIX}/libcrypto.a \
	${LIB_PREFIX}/libssl.a \
	-lrt \
	-ldl \
	-lpthread

SOURCES := \
    client.c \
    client_api.c \
    watch_path.c

# see xsyncdef.h for definitions
SRC_DEFS := DEBUG \
	APP_NAME='"${TARGET}"' \
	INEVENT_BUFSIZE=4096


SRC_INCDIRS := \
    . \
	.. \
	../common
