########################################################################
# server.mk
#
# update: 2018-01-24
########################################################################

# !!! DO NOT CHANGE TARGET IF YOU MAKE SURE WHAT YOU ARE DOING !!!
TARGET := xsync-server

VERSION := 0.0.1


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
	XSYNC_SERVER_APPVER='"${VERSION}"'


SRC_INCDIRS := \
    . \
	.. \
	../common
