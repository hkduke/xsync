#######################################################################
# @file: main.mk
#
# @version: 0.1.8
# @create: 2018-05-18 14:00:00
# @update: 2018-10-15 13:10:51
#######################################################################
# The libredisapi.a library can normally be built by itself (without also
#   building the "xsync" program) by running "make libredisapi.a" from
#   within the "src" directory.
#
# This main.mk exists solely for the purpose of also allowing users to
#   build the libredisapi.a library by itself by running "make" from
#   within the "redisapi" subdirectory.

BUILD_DIR  := ../../build

TARGET_DIR := ../../target

CFLAGS := -std=gnu99 -g -O0 -Wall -pipe

SUBMAKEFILES := redisapi.mk
