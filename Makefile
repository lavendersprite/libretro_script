# Toolset
CC=gcc
CXX=g++
INCLUDES=-Iinclude -Ideps -Ideps/lua-5.4.3

CFLAGS=$(INCLUDES) $(DEFINES) -Wall -Werror -fPIC
CXXFLAGS=$(CFLAGS) -std=c++11
LDFLAGS=-shared -lm

ifeq ($(DEBUG), 1)
	CFLAGS += -g -DRETRO_SCRIPT_DEBUG
else
	CFLAGS += -O2
endif

ifeq ($(OS),Windows_NT)
	LIB_SUFFIX=.lib
	LIB_PREFIX=
    SHLIB_SUFFIX=.dll
	SHLIB_PREFIX=
else
	LIB_PREFIX=lib
	LIB_SUFFIX=.a
    SHLIB_SUFFIX=.so
	SHLIB_PREFIX=lib
	CFLAGS += -DLUA_USE_POSIX
endif

SHLIB=$(SHLIB_PREFIX)retro_script$(SHLIB_SUFFIX)
LIB=$(LIB_PREFIX)retro_script$(LIB_SUFFIX)

SCRIPT_SOURCES=$(wildcard src/*.c)
SCRIPT_OBJECTS=$(patsubst %.c,%.o,$(SCRIPT_SOURCES))
LUA_SOURCES=$(wildcard deps/lua-5.4.3/*.c)
LUA_OBJECTS=$(patsubst %.c,%.o,$(LUA_SOURCES))

OBJECTS=$(LUA_OBJECTS) $(SCRIPT_OBJECTS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(SHLIB): $(OBJECTS)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	
$(LIB): $(OBJECTS)
	ar rcs $(LIB) $(OBJECTS)
	
lib: $(LIB)

shlib: $(SHLIB)

all: $(SHLIB) $(LIB)

clean:
	rm -f $(SCRIPT_OBJECTS) $(LUA_OBJECTS) $(SHLIB) $(LIB)

clean-lib:
	rm -f $(SCRIPT_OBJECTS) $(SHLIB) $(LIB)

.PHONY: clean all lib shlib