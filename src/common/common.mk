#######################################################################
# @file: common.mk
#
# @version: 0.4.4
# @create: 2012-05-18 14:00:00
# @update: 2018-11-07 10:20:15
#######################################################################
TARGET := libcommon.a

SOURCES := \
	log4c_logger.c \
	threadpool.c \
	getopt_longw.c \
	getoptw.c \
	base64.c \
	readconf.c \
	mul_timer.c \
	randctx.c \
	rc4.c \
	red_black_tree.c


#   If the macro NDEBUG is defined at the moment <assert.h> was last
#     included, the macro assert() generates no code, and hence does
#     nothing at all.
SRC_DEFS := NDEBUG


SRC_INCDIRS := . \
	../../libs/include

# common has its own submakefile because it has a specific SRC_DEFS that we
# want to apply only to it
# SUBMAKEFILES := path/to/sub.mk


define INSTALL_LIBCOMMON_A
	mv ${TARGET_DIR}/${TARGET} ${TARGET_DIR}/../libs/lib/
endef

TGT_POSTMAKE := ${INSTALL_LIBCOMMON_A}

