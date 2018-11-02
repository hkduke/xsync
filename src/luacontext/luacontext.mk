#######################################################################
# @file: luacontext.mk
#
# @version: 0.3.9
# @create: 2018-10-10 10:00:00
# @update: 2018-10-29 10:24:55
#######################################################################

TARGET := libluacontext.a

LIB_PREFIX := ${TARGET_DIR}/../libs/lib

TGT_LDFLAGS := -L${TARGET_DIR}

TGT_LDLIBS  := \
	${LIB_PREFIX}/liblua.a \
	-lpthread


SOURCES := \
	luacontext.c


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


define INSTALL_LIBLUACONTEXT_A
	mv ${TARGET_DIR}/${TARGET} ${TARGET_DIR}/../libs/lib/
endef

TGT_POSTMAKE := ${INSTALL_LIBLUACONTEXT_A}

