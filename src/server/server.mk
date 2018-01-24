########################################################################
# server.mk
#
# update: 2018-01-24
########################################################################

# !!! DO NOT change APPNAME and VERSION only when you make sure do that !
APPNAME := xsync-server
VERSION := 0.0.1

TARGET := ${APPNAME}-${VERSION}


LIB_PREFIX := ${TARGET_DIR}/../libs/lib

TGT_LDFLAGS := -L${TARGET_DIR}

TGT_LDLIBS  := \
	${LIB_PREFIX}/libcommon.a \
	${LIB_PREFIX}/liblog4c.a

SOURCES := \
    server.c


# see "../xsync-config.h" for definitions
SRC_DEFS := DEBUG \
	XSYNC_SERVER_APPNAME='"${TARGET}"' \
	XSYNC_SERVER_VERSION='"${VERSION}"'


SRC_INCDIRS := \
    . \
	.. \
	../common
