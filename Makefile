# Toolset
CC=gcc
CXX=g++
INCLUDES=\
	-Iinclude -Ihackable-console/include -IRetroArch/libretro-common/include

CFLAGS=$(INCLUDES) $(DEFINES) -Wall -Werror
CXXFLAGS=$(CFLAGS) -std=c++11
LDFLAGS=-shared

ifeq ($(OS),Windows_NT)
    SHLIB_SUFFIX=.dll
	SHLIB_PREFIX=
else
    SHLIB_SUFFIX=.so
	SHLIB_PREFIX=lib
endif

SHLIB=$(SHLIB_PREFIX)retro-script$(SHLIB_SUFFIX)

SCRIPT_SOURCES=$(wildcard src/*.cpp)
SCRIPT_OBJECTS=$(patsubst %.cpp,%.o,$(SCRIPT_SOURCES))

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CXXFLAGS) -c $< -o $@

$(SHLIB): $(SCRIPT_OBJECTS)
	$(CC) $(SCRIPT_OBJECTS) -o $@ $(LDFLAGS)

clean:
	rm -f $(SCRIPT_OBJECTS) $(SHLIB)

.PHONY: clean
