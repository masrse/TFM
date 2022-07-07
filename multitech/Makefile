# Path absoluto o relativo al stack del programa server
STACK_PATH := $(shell dirname $(word $(words $(MAKEFILE_LIST)),$(MAKEFILE_LIST)))

OS_VERSION = $(shell . /etc/os-release; echo $$VERSION | sed s'/[() ]//g')

SHELL := /bin/bash

# CC = g++
CC = arm-mlinux-linux-gnueabi-g++  -march=armv5te -marm -mthumb-interwork -mtune=arm926ej-s --sysroot=/opt/mlinux/5.0.0/sysroots/arm926ejste-mlinux-linux-gnueabi
CC_TEST = g++

CPPFLAGS = -Wall -Wextra -g -std=c++11 -lpthread -lcurl
CPPFLAGS_TEST = -Wall -g -std=c++11 -fno-exceptions --coverage -mno-ms-bitfields -DARDUINO=182 -D_UCRT -DTEST_FLAG

BINARY = prod_multitech

TEST_TARGET = multitech_test

#Ruta absoluta del makefile
mkfile_path := $(abspath $(lastword $(MAKEFILE_LIST)))
mkfile_dir := $(dir $(mkfile_path))

INCLUDES_TEST += \
	-Isrc/\

INCLUDES += \
	-I$(mkfile_dir)src/ \
	-I$(mkfile_dir)../../libs/common/wtc_util/src \
	-I$(mkfile_dir)../../libs/common/wtc_base/src \
	-I$(mkfile_dir)../../libs/test/avr-mock/include \
	-I$(mkfile_dir)../../libs/common/crc/src \
	-I$(mkfile_dir)../../libs/server/log/inc \
	-I$(mkfile_dir)../../libs/device/wtc_satellital/src \
	-I$(mkfile_dir)../../libs/common/wtc_models/src \
	-I$(mkfile_dir)../../libs/common/data_formatter/src \
	-I$(mkfile_dir)../../libs/device/pkt_filter/src \

TEST_INCLUDES += \
	-I/usr/src/googletest/googletest/include/ \
	-I/usr/src/googletest/googlemock/include/ \

LIBS += \
	$(mkfile_dir)src/*.cpp \
	$(mkfile_dir)../../libs/common/wtc_util/src/*.cpp \
	$(mkfile_dir)../../libs/common/wtc_base/src/wtc_base.cpp \
	$(mkfile_dir)../../libs/common/wtc_base/src/pkt.cpp \
	$(mkfile_dir)../../libs/common/crc/src/*.cpp \
	$(mkfile_dir)../../libs/server/log/src/*.cpp \
	$(mkfile_dir)../../libs/device/wtc_satellital/src/*.cpp \
	$(mkfile_dir)../../libs/common/wtc_models/src/*.cpp \
	$(mkfile_dir)../../libs/common/data_formatter/src/*.cpp \
	$(mkfile_dir)../../libs/device/pkt_filter/src/*.cpp \

# Codigo fuente
SRC = $(wildcard $(LIBS))
SRCS = src/imei_list.cpp
TEST_SRCS := $(shell find test/ -type f -name '*.cpp')

OBJS = $(SRC:%.cpp=%.o)

TEST_LIBS += \
	-lgtest \
	-lgtest_main \
	-lgmock \
	-lgmock_main \
	-lpthread \

all: lora_tcp_server.o main.o
	$(CC) $(CPPFLAGS) *.o -o $(BINARY)

lora_tcp_server.o: $(SRC)
	$(CC) $(CPPFLAGS) $(INCLUDES) -c $^

main.o: main.cpp
	$(CC) $(CPPFLAGS) -c $^ $(INCLUDES)

show:
	@echo $(mkfile_dir)

wtc_test:
	$(CC_TEST) $(CPPFLAGS_TEST) -c $(INCLUDES_TEST) $(TEST_INCLUDES) $(TEST_SRCS) $(SRCS)
	$(CC_TEST) $(CPPFLAGS_TEST) $(INCLUDES_TEST) $(TEST_INCLUDES) *.o -o $(TEST_TARGET) $(TEST_LIBS)

run_test:
	./$(TEST_TARGET)

clean:
	rm -rf *.o
	rm -rf *.gch
	rm -rf $(BINARY)
	rm -rf $(mkfile_dir)/src/*.o
	rm -rf $(mkfile_dir)/../../libs/common/wtc_base/src/*.o
	rm -rf $(mkfile_dir)/../../libs/common/wtc_util/src/*.o
	rm -rf deploy_multitech
	rm -f $(TEST_TARGET)

package_multitech:
	mkdir -p deploy_multitech/$(OS_VERSION)
	cp prod_multitech deploy_multitech/$(OS_VERSION)
