LIB = libmspsoftuart

OBJECTS = \
	uart.o \

override SRC_ROOT = ../../src

DEPS += libmsp
CFLAGS += -I$(SRC_ROOT)/include/$(LIB)

include $(MAKER_ROOT)/Makefile.gcc
