# The xsync-client application can normally be built by itself.
#
# This main.mk exists solely for the purpose of also allowing users to
# build the xsync-client application by itself by running "make" from
# within the "client" subdirectory.

BUILD_DIR  := ../../build
TARGET_DIR := ../../target

CFLAGS := -std=c99 -g -O0 -Wall -pipe

SUBMAKEFILES := client.mk
