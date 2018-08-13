########################################################################
# server.mk
#
# update: 2018-01-24
########################################################################

# !!! DO NOT change APPNAME and VERSION only when you make sure do that !
APPNAME := xsync-server
VERSION := 0.0.4

TARGET := ${APPNAME}-${VERSION}


LIB_PREFIX := ${TARGET_DIR}/../libs/lib


# how to ld libzdb.so
# see:
#   http://blog.csdn.net/21aspnet/article/details/6724457
#
#   '-Wl,-rpath,'
#
TGT_LDFLAGS := \
	-L${TARGET_DIR} \
	-L${TARGET_DIR}/../libs/lib \
	-Wl,-rpath,${TARGET_DIR}/../libs/lib


TGT_LDLIBS  := \
	${LIB_PREFIX}/libcommon.a \
	${LIB_PREFIX}/liblog4c.a \
	${LIB_PREFIX}/libexpat.a \
	${LIB_PREFIX}/libsqlite3.a \
	${LIB_PREFIX}/libjemalloc.a \
	${LIB_PREFIX}/libmxml.a \
	${LIB_PREFIX}/libcrypto.a \
	${LIB_PREFIX}/libssl.a \
	${LIB_PREFIX}/libhiredis.a \
	${LIB_PREFIX}/libz.a \
	-lzdb \
	-lrt \
	-ldl \
	-lpthread


SOURCES := \
    server.c \
    server_api.c \
    server_conf.c \
    client_session.c \
    file_entry.c


# see "../xsync-config.h" for definitions
#   If the macro NDEBUG is defined at the moment <assert.h> was last
#     included, the macro assert() generates no code, and hence does
#     nothing at all.
SRC_DEFS := DEBUG \
	XSYNC_SERVER_APPNAME='"${APPNAME}"' \
	XSYNC_SERVER_VERSION='"${VERSION}"' \
    XSYNC_SERVER_THREADS_MAX=128 \
	XSYNC_SERVER_QUEUES_MAX=4096


SRC_INCDIRS := \
    . \
	.. \
	../common
