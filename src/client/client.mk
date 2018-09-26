#######################################################################
# @file: client.mk
#   see: "client.mk.another" for another style of client.mk
#
# @version: 0.0.7
# @create: 2018-05-18 14:00:00
# @update: 2018-09-26 10:55:32
#######################################################################

# !!! DO NOT change APPNAME and VERSION only when you make sure do that !
APPNAME := xsync-client
VERSION := 0.0.7

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
	${LIB_PREFIX}/libz.a \
	${LIB_PREFIX}/libinotifytools.a \
	-lrt \
	-ldl \
	-lpthread

SOURCES := \
    client.c \
    client_api.c \
    client_conf.c \
    watch_path.c \
    watch_event.c \
    watch_entry.c \
    path_filter.c \
	server_conn.c


# see "../xsync-config.h" for definitions
#   If the macro NDEBUG is defined at the moment <assert.h> was last
#     included, the macro assert() generates no code, and hence does
#     nothing at all.
#
#  XSYNC_CLIENT_THREADS=4
#  XSYNC_CLIENT_QUEUES=256
#
SRC_DEFS := DEBUG \
	XSYNC_CLIENT_APPNAME='"${APPNAME}"' \
	XSYNC_SERVER_VERSION='"${VERSION}"' \
	XSYNC_PATH_MAXSIZE=1024 \
	XSYNC_INEVENT_BUFSIZE=4096 \
	XSYNC_SERVER_MAXID=32


SRC_INCDIRS := \
    . \
	.. \
	../common
