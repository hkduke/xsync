# The xsync-server application can normally be built by itself.
#
# This main.mk exists solely for the purpose of also allowing users to
# build the xsync-server application by itself by running "make" from
# within the "server" subdirectory.

INCDIRS := \
	../../libs/include \
	../../libs/include/zdb


TGT_LDLIBS :=

BUILD_DIR  := ../../build
TARGET_DIR := ../../target

CFLAGS := -std=gnu99 -g -O0 -Wall -pipe

SUBMAKEFILES := server.mk
