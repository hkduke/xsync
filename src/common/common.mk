TARGET := libcommon.a

SOURCES := \
	threadpool.c \
	getopt_longw.c \
	getoptw.c \
	base64.c \
	readconf.c \
	mul_timer.c \
	randctx.c


#   If the macro NDEBUG is defined at the moment <assert.h> was last
#     included, the macro assert() generates no code, and hence does
#     nothing at all.
SRC_DEFS := NDEBUG


SRC_INCDIRS := .


# common has its own submakefile because it has a specific SRC_DEFS that we
# want to apply only to it
# SUBMAKEFILES := path/to/sub.mk


define INSTALL_LIBCOMMON_A
	 mv ${TARGET_DIR}/${TARGET} ${TARGET_DIR}/../libs/lib/
endef

TGT_POSTMAKE := ${INSTALL_LIBCOMMON_A}

