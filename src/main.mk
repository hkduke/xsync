
CFLAGS := -g -O0 -Wall -std=gnu99 -pipe

INCDIRS := ../libs/include

BUILD_DIR  := ../build
TARGET_DIR := ../target

SUBMAKEFILES := common/common.mk server/server.mk client/client.mk
