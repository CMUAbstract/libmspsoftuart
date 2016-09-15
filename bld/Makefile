LIB = libmspsoftuart

OBJECTS = \
	uart.o \

DEPS += \
	libmsp \

override SRC_ROOT = ../../src

override CFLAGS += \
	-I$(SRC_ROOT)/include/$(LIB) \

include ../Makefile.config
include ../Makefile.options
