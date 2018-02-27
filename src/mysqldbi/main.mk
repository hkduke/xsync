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
