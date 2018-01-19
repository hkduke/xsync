
CFLAGS := -g -O0 -Wall -std=c99 -pipe

INCDIRS := ../libs/include

BUILD_DIR  := ../build
TARGET_DIR := ../target

SUBMAKEFILES := server/server.mk client/client.mk common/common.mk
