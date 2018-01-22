# GCC, the GNU C compiler, supports `-g' with or without `-O',
#   making it possible to debug optimized code.
# We recommend that you always use `-g' whenever you compile a program.
# You may think your program is correct, but there is no sense in
#   pushing your luck.

CFLAGS := -g -O0 -Wall -std=gnu99 -pipe

INCDIRS := ../libs/include

BUILD_DIR  := ../build
TARGET_DIR := ../target

SUBMAKEFILES := common/common.mk server/server.mk client/client.mk
