#######################################################################
# @file: redisapi.mk
#
# @version: 0.4.4
# @create: 2018-05-18 14:00:00
# @update: 2018-11-07 10:20:15
#######################################################################

TARGET := libredisapi.a

LIB_PREFIX := ${TARGET_DIR}/../libs/lib

TGT_LDFLAGS := -L${TARGET_DIR}

TGT_LDLIBS  := \
	${LIB_PREFIX}/libhiredis.a \
	-lpthread


SOURCES := \
	redis_api.c


#   If the macro NDEBUG is defined at the moment <assert.h> was last
#     included, the macro assert() generates no code, and hence does
#     nothing at all.
SRC_DEFS := NDEBUG


SRC_INCDIRS := . \
    ../../libs/include


# It has its own submakefile because it has a specific SRC_DEFS that we
#   want to apply only to it:
#
# SUBMAKEFILES := path/to/sub.mk


define INSTALL_LIBREDISAPI_A
	mv ${TARGET_DIR}/${TARGET} ${TARGET_DIR}/../libs/lib/
endef

TGT_POSTMAKE := ${INSTALL_LIBREDISAPI_A}

