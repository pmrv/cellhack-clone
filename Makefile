CFLAGS=-O2 -Ilib -rdynamic `pkg-config --cflags sdl2` $(OPTFLAGS)
LDLIBS=-ldl -lm `pkg-config --libs sdl2` $(OPTLIBS)

LIB_SOURCES=$(wildcard lib/**/*.c)
LIB_OBJECTS=$(patsubst %.c,%.o,$(LIB_SOURCES))

EXE_SOURCES=$(wildcard bin/*.c)
EXE_TARGETS=$(patsubst %.c,%,$(EXE_SOURCES))

MSC_SOURCES=$(wildcard lib/*.c)
MSC_OBJECTS=$(patsubst %.c,%.o,$(MSC_SOURCES))

EXAMPLES=$(wildcard examples/*.c)
SO_EXAMPLES=$(patsubst %.c,%.so,$(EXAMPLES))

TARGET=build/libcellhack.a
SO_TARGET=$(patsubst %.a,%.so,$(TARGET))

dev: CFLAGS+=-g -Wall -Wextra -O0 -DDEBUG
dev: all

all: $(TARGET) $(SO_TARGET) $(EXE_TARGETS) examples

examples: $(SO_EXAMPLES)

%.so: CFLAGS+=-fPIC --shared
%.so: %.c
	gcc $(CFLAGS) -o $@ $*.c

$(EXE_TARGETS): build $(MSC_OBJECTS) $(TARGET)
	$(CC) $(CFLAGS) -o $@ $@.c $(MSC_OBJECTS) $(TARGET) $(LDLIBS)

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
	rm -rf build $(TARGET) $(SO_TARGET) $(LIB_OBJECTS) $(EXE_TARGETS) $(SO_EXAMPLES)
