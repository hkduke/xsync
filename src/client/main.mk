#######################################################################
# @file: main.mk
#   see: "client.mk.another" for another style of client.mk
#
# @version: 0.4.4
# @create: 2018-05-18 14:00:00
# @update: 2018-11-07 10:20:15
#######################################################################
# The xsync-client application can normally be built by itself.
#
# This main.mk exists solely for the purpose of also allowing users to
# build the xsync-client application by itself by running "make" from
# within the "client" subdirectory.
INCDIRS := ../../libs/include

BUILD_DIR  := ../../build
TARGET_DIR := ../../target

CFLAGS := -std=gnu99 -g -O0 -Wall -pipe

SUBMAKEFILES := client.mk
