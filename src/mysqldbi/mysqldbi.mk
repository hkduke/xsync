#######################################################################
# @file: mysqldbi.mk
#
# @version: 0.1.1
# @create: 2018-05-18 14:00:00
# @update: 2018-08-29 13:34:19
#######################################################################
TARGET := libmysqldbi.a

SOURCES := \
	mysqldbi.c


#   If the macro NDEBUG is defined at the moment <assert.h> was last
#     included, the macro assert() generates no code, and hence does
#     nothing at all.
SRC_DEFS := NDEBUG


SRC_INCDIRS := .


# It has its own submakefile because it has a specific SRC_DEFS that we
#   want to apply only to it:
#
# SUBMAKEFILES := path/to/sub.mk


define INSTALL_LIBMYSQLDBI_A
	 mv ${TARGET_DIR}/${TARGET} ${TARGET_DIR}/../libs/lib/
endef

TGT_POSTMAKE := ${INSTALL_LIBMYSQLDBI_A}

