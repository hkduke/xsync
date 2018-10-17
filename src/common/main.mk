#######################################################################
# @file: main.mk
#
# @version: 0.1.6
# @create: 2012-05-18 14:00:00
# @update: 2018-08-29 13:34:50
#######################################################################
# The libcommon.a library can normally be built by itself (without also
# building the "xsync" program) by running "make libcommon.a" from within the
# "src" directory.
#
# This main.mk exists solely for the purpose of also allowing users to build the
# libcommon.a library by itself by running "make" from within the "common"
# subdirectory.

BUILD_DIR  := ../../build

TARGET_DIR := ../../target

CFLAGS := -std=gnu99 -g -O0 -Wall -pipe

LDFLAGS := -lpthread

SUBMAKEFILES := common.mk
