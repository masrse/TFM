include ../../../Makefile.defs

.PHONY: clean install

# Target
TARGET = libwtc_lorawan.a
TEST_TARGET = test_wtc_lorawan

# Compilador
CC = g++

# Carpetas con includes
INCLUDES += \
	-I$(INSTALLPATH)/usr/local/inc/ \
	$(TEST_INCLUDES) \
	-Iscr/

# Codigo fuente
SRCS = $(wildcard src/*.cpp)
TEST_SRCS := $(shell find test/ -type f -name '*.cpp')
TEST_SRCS += ../../test/test_devices_main.cpp

# Headers
HDRS = $(wildcard src/*.h)

# Codigo objeto
OBJS = $(SRCS:.cpp=.o)
TEST_OBJS = $(TEST_SRCS:.cpp=.o)

# Flags al compilador
CPPFLAGS = -c -Wall -g -std=c++11

# Flags al linker
LDFLAGS += -g \

# Carpetas con librerias
LDIR = $(INSTALLPATH)/usr/local/lib

# Librerias
LIBS += \
	-lwtc_lorawan \
	-lwtc_network \
	-lwtc_serial \
	-ldata_formatter \
	-lwtc_util \
	-lpkt_filter \
	-lwtc_comms \
	-lwtc_base \
	-lcrc \
	-lwtc_models \
	-larduino-mock \
	-lavr-mock \
	-lwtc-mock \
	-lgtest \
	-lgmock \
	-lpthread \

$(TARGET): $(OBJS)
	ar cr $(TARGET) $(OBJS)

$(TEST_TARGET): $(TEST_OBJS) $(LIB_OBJS)
	ar cr  $(TEST_TARGET) $(TEST_OBJS)

%.o: %.cpp
	$(CC) $(CPPFLAGS) $(TEST_FLAGS) $(INCLUDES) $< -c -o $@

wtc_test: $(TEST_TARGET)
	$(CC) $(LDFLAGS) $(TEST_FLAGS) -L$(LDIR) $(TEST_OBJS) -o $(TEST_TARGET) $(LIBS)

run_test:
	./$(TEST_TARGET)

clean:
	rm -f $(OBJS) $(TEST_OBJS) $(TARGET) $(TEST_TARGET)

install_hdr:
	for hdr in $(HDRS); do install -D $$hdr $(INSTALLPATH)/usr/local/inc/`basename $$hdr`; done

install_lib:
	install -D $(TARGET) $(INSTALLPATH)/usr/local/lib/$(TARGET)
