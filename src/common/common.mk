TARGET := libcommon.a

SOURCES := \
	threadpool.c

SRC_INCDIRS := .

# common has its own submakefile because it has a specific SRC_DEFS that we
# want to apply only to it
# SUBMAKEFILES := path/to/sub.mk
