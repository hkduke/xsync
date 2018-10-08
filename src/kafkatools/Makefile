# Makefile for libkafkatools.so

major_verno = 1
minor_verno = 0
revision_no = 0

# DO NOT change below :
#
prefix  = .
binname = libkafkatools
version = ${major_verno}.${minor_verno}.${revision_no}
bintarget = ${binname}.so.${version}

CFLAGS ?= -std=gnu99 -g -O0 -Wall -pipe -fPIC

LDFLAGS ?= -L${prefix}/../../libs/lib -lrdkafka -lz -lpthread -lrt

INC_DIRS ?= -I. \
	-I${prefix}/../../libs/include


ifdef DEBUG
CFLAGS += -g
LDFLAGS += -g
endif


all: $(bintarget)
	-cp ${bintarget} ../../target/
	-ln -sf ${bintarget} ../../target/${binname}.so


$(bintarget): kafkatools.o
	$(CC) $(CFLAGS) -shared \
		-Wl,--soname=${binname}.so.${major_verno} \
		-Wl,--rpath='${prefix}:${prefix}/lib:${prefix}/../lib:${prefix}/../libs/lib' \
		-o $@ \
		*.o \
		$(LDFLAGS)

kafkatools.o: kafkatools.c
	$(CC) $(CFLAGS) $(INC_DIRS) -c kafkatools.c -o $@

clean:
	-rm -f $(bintarget)
	-rm -f kafkatools.o
	-rm -f ../../target/${binname}.so
	-rm -f ../../target/${bintarget}

.PHONY: all
