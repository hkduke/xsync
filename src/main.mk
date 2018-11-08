#######################################################################
# @file: main.mk
#   refer: https://github.com/dmoulding/boilermake
#
# @version: 0.4.3
# @create: 2018-05-18 14:00:00
# @update: 2018-11-07 10:20:15
#######################################################################
# GCC, the GNU C compiler, supports `-g' with or without `-O',
#   making it possible to debug optimized code.
# We recommend that you always use `-g' whenever you compile a program.
# You may think your program is correct, but there is no sense in
#   pushing your luck.
#
# If the macro NDEBUG is defined at the moment <assert.h> was last
#     included, the macro assert() generates no code, and hence does
#     nothing at all.
#
########################################################################
# Build for release, MUST use the following:
#       -DMULTIMER_PRINT=0
#       -DNDEBUG

# Build for RELEASE, SHOULD use one of the following:
#       -DLOGGER_LEVEL_INFO
#       -DLOGGER_LEVEL_WARN
#       -DLOGGER_LEVEL_ERROR

# Build for DEBUG:
CFLAGS := -g -O0 -Wall -std=gnu99 -pipe -DLOGGER_LEVEL_TRACE -DDEBUG


# Build for RELEASE:
#CFLAGS := -g -O0 -Wall -std=gnu99 -pipe -DMULTIMER_PRINT=0 -DLOGGER_LEVEL_INFO -DNDEBUG


INCDIRS := \
	../libs/include \
	../libs/include/zdb


BUILD_DIR  := ../build

TARGET_DIR := ../target

SUBMAKEFILES := common/common.mk \
	luacontext/luacontext.mk \
	redisapi/redisapi.mk \
	server/server.mk \
	client/client.mk
