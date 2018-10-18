#######################################################################
# @file: main.mk
#
# @version: 0.1.7
# @create: 2018-05-18 14:00:00
# @update: 2018-08-29 13:34:19
#######################################################################
# The libmysqldbi.a library can normally be built by itself (without also
#   building the "xsync" program) by running "make libmysqldbi.a" from
#   within the "src" directory.
#
# This main.mk exists solely for the purpose of also allowing users to
#   build the libmysqldbi.a library by itself by running "make" from
#   within the "mysqldbi" subdirectory.

BUILD_DIR  := ../../build
TARGET_DIR := ../../target

CFLAGS := -std=gnu99 -g -O0 -Wall -pipe `mysql_config --cflags`

LDFLAGS := `mysql_config --libs`

SUBMAKEFILES := mysqldbi.mk
