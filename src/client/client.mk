########################################################################
# client.mk
#
# see: "client.mk.another" for another style of client.mk
#
# update: 2018-01-24
########################################################################

# !!! DO NOT change APPNAME and VERSION only when you make sure do that !
APPNAME := xsync-client
VERSION := 0.0.1

TARGET := ${APPNAME}-${VERSION}


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
    watch_path.c \
    watch_event.c \
    path_filter.c


# see "../xsync-config.h" for definitions
SRC_DEFS := DEBUG \
	XSYNC_CLIENT_APPNAME='"${APPNAME}"' \
	XSYNC_SERVER_VERSION='"${VERSION}"' \
	XSYNC_INEVENT_BUFSIZE=4096 \
	XSYNC_SERVER_MAXID=10 \
	XSYNC_CLIENT_THREADS_MAX=16 \
	XSYNC_CLIENT_QUEUES_MAX=256


SRC_INCDIRS := \
    . \
	.. \
	../common
