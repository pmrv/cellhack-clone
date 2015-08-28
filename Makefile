CFLAGS=-O2 -Isrc -rdynamic `pkg-config --cflags sdl2` $(OPTFLAGS)
LDLIBS=-ldl -lm `pkg-config --libs sdl2` $(OPTLIBS)

LIB_SOURCES=$(wildcard src/**/*.c)
LIB_OBJECTS=$(patsubst %.c,%.o,$(LIB_SOURCES))

EXE_SOURCES=$(wildcard src/*.c)
EXE_OBJECTS=$(patsubst %.c,%.o,$(EXE_SOURCES))

EXE_TARGET=build/cellhack

EXAMPLES=$(wildcard examples/*.c)
SO_EXAMPLES=$(patsubst %.c,%.so,$(EXAMPLES))

TARGET=build/libcellhack.a
SO_TARGET=$(patsubst %.a,%.so,$(TARGET))

dev: CFLAGS+=-g -Wall -Wextra -O0
dev: all

all: $(TARGET) $(SO_TARGET) $(EXE_TARGET) $(AI_SOURCES) examples

examples: $(SO_EXAMPLES)

%.so: %.c
	gcc $(CFLAGS) --shared -o $@ $*.c

$(EXE_TARGET): build $(EXE_OBJECTS) $(TARGET)
	$(CC) $(CFLAGS) -o $(EXE_TARGET) $(EXE_OBJECTS) $(TARGET) $(LDLIBS)

$(TARGET): CFLAGS += -fPIC
$(TARGET): build $(LIB_OBJECTS)
	ar rcs $@ $(LIB_OBJECTS)
	ranlib $@

$(SO_TARGET): $(TARGET) $(LIB_OBJECTS)
	$(CC) --shared -o $@ $(LIB_OBJECTS)

build:
	@mkdir -p build

.PHONY:
clean:
	rm -rf build $(LIB_OBJECTS) $(EXE_OBJECTS) $(SO_EXAMPLES)
