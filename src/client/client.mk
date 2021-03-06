#######################################################################
# @file: client.mk
#   see: "client.mk.another" for another style of client.mk
#
# @version: 0.4.4
# @create: 2018-05-18 14:00:00
# @update: 2018-11-09 17:06:23
#######################################################################
prefix = .

# !!! DO NOT change APPNAME and VERSION only when you make sure do that !
APPNAME := xsync-client
VERSION := 0.4.4

TARGET := ${APPNAME}-${VERSION}


LIB_PREFIX := ${TARGET_DIR}/../libs/lib

TGT_LDFLAGS := -L${TARGET_DIR} -L${LIB_PREFIX}/lua/5.3 \
	-Wl,--soname=cjson.so \
	-Wl,--rpath='./lib:../lib/lua/5.3:${LIB_PREFIX}/lua/5.3'

# ldd xsync-client
# readelf -d xsync-client
#
TGT_LDLIBS  := \
	${LIB_PREFIX}/libcommon.a \
	${LIB_PREFIX}/liblog4c.a \
	${LIB_PREFIX}/libexpat.a \
	${LIB_PREFIX}/libcrypto.a \
	${LIB_PREFIX}/libssl.a \
	${LIB_PREFIX}/libz.a \
	${LIB_PREFIX}/libinotifytools.a \
	${LIB_PREFIX}/libluacontext.a \
	${LIB_PREFIX}/liblua.a \
	${LIB_PREFIX}/libjemalloc.a \
	-lcjson \
	-lm \
	-lrt \
	-ldl \
	-lpthread


SOURCES := \
	inotifyapi.c \
	client.c \
	client_api.c \
	client_conf.c \
	watch_entry.c \
	server_conn.c


# see "../xsync-config.h" for definitions
#   If the macro NDEBUG is defined at the moment <assert.h> was last
#     included, the macro assert() generates no code, and hence does
#     nothing at all.
#
#  XSYNC_CLIENT_THREADS=4
#  XSYNC_CLIENT_QUEUES=256
#  XSYNC_USE_STATIC_PATHID_TABLE
#
SRC_DEFS := NDEBUG \
	XSYNC_CLIENT_APPNAME='"${APPNAME}"' \
	XSYNC_SERVER_VERSION='"${VERSION}"' \
	XSYNC_PATH_MAXSIZE=1024 \
	XSYNC_INEVENT_BUFSIZE=4096 \
	XSYNC_SERVER_MAXID=32 \
	XSYNC_WATCH_PATHID_MAX=256 \
    MEMAPI_USE_LIBJEMALLOC


SRC_INCDIRS := \
    . \
	.. \
	../common
