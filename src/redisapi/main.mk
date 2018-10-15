#######################################################################
# @file: main.mk
#
# @version: 0.1.2
# @create: 2018-05-18 14:00:00
# @update: 2018-08-29 13:33:58
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
