LIB = libmspsoftuart
EXEC = $(LIB).a

OBJECTS = \
	uart.o \

DEPS += libmsp
CFLAGS += -I$(SRC_DIR)/include/$(LIB)

SRC_DIR = ../../src
VPATH = $(SRC_DIR)

include $(MAKER_ROOT)/Makefile.gcc
