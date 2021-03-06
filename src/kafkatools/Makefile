# Makefile for libkafkatools.so

major_verno = 1
minor_verno = 0
revision_no = 0

# DO NOT change below :
#
prefix  = .
binname = libkafkatools
version = $(major_verno).$(minor_verno).$(revision_no)
bintarget = $(binname).so.$(version)
binsoname = $(binname).so.$(major_verno)

LIB_DIR = $(prefix)/../../libs/lib

CFLAGS ?= -std=gnu99 -D_GNU_SOURCE -O0 -Wall -pipe -fPIC

LDFLAGS ?= -L$(LIB_DIR) -lrdkafka -lz -lpthread -lrt

INC_DIRS ?= -I. \
	-I$(prefix)/../../libs/include


ifdef DEBUG
CFLAGS += -g
LDFLAGS += -g
endif


all: $(bintarget)
	-mkdir -p ../../target
	-cp $(bintarget) ../../target/
	-ln -sf $(bintarget) ../../target/$(binsoname)


$(bintarget): kafkatools_consumer.o kafkatools_producer.o red_black_tree.o
	$(CC) $(CFLAGS) -shared \
		-Wl,--soname=$(binsoname) \
		-Wl,--rpath='$(prefix):$(prefix)/lib:$(prefix)/../lib:$(prefix)/../libs/lib' \
		-o $@ \
		*.o \
		$(LDFLAGS)

kafkatools_consumer.o: kafkatools_consumer.c
	$(CC) $(CFLAGS) $(INC_DIRS) -c kafkatools_consumer.c -o $@

kafkatools_producer.o: kafkatools_producer.c
	$(CC) $(CFLAGS) $(INC_DIRS) -c kafkatools_producer.c -o $@

red_black_tree.o: red_black_tree.c
	$(CC) $(CFLAGS) $(INC_DIRS) -c red_black_tree.c -o $@


clean:
	-rm -f $(prefix)/*.o
	-rm -f $(prefix)/$(bintarget)
	-rm -f $(prefix)/../../target/$(binsoname)
	-rm -f $(prefix)/../../target/$(bintarget)

.PHONY: all
