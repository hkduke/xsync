########################################################################
# client.mk
#
# see: "client.mk.another" for another style of client.mk
#
# update: 2018-01-24
########################################################################

# !!! DO NOT CHANGE TARGET IF YOU MAKE SURE WHAT YOU ARE DOING !!!
TARGET := xsync-client
VERSION := 0.0.1


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


# see "../xsync-config.h" for definitions
SRC_DEFS := DEBUG \
	XSYNC_CLIENT_APPNAME='"${TARGET}"' \
	XSYNC_CLIENT_APPVER='"${VERSION}"' \
	XSYNC_INEVENT_BUFSIZE=4096


SRC_INCDIRS := \
    . \
	.. \
	../common
