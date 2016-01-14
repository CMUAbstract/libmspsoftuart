LIB = libmspsoftuart
EXEC = $(LIB).a

OBJECTS = \
	uart.o \

include $(MAKER_ROOT)/Makefile.gcc

SRC_DIR = ../../src

CFLAGS += -I$(SRC_DIR)/include/$(LIB)

$(eval $(call depend,libmsp))

VPATH = $(SRC_DIR)
