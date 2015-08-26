CFLAGS=-O2 -Isrc -rdynamic $(OPTFLAGS)
LDLIBS=-ldl -lm $(OPTLIBS)
PREFIX?=/usr/local

LIB_SOURCES=$(wildcard src/**/*.c)
LIB_OBJECTS=$(patsubst %.c,%.o,$(LIB_SOURCES))

EXE_SOURCES=$(wildcard src/*.c)
EXE_OBJECTS=$(patsubst %.c,%.o,$(EXE_SOURCES))

EXE_TARGET=build/cellhack

TEST_SRC=$(wildcard tests/*_tests.c)
TESTS=$(patsubst %.c,%,$(TEST_SRC))

TARGET=build/libcellhack.a
SO_TARGET=$(patsubst %.a,%.so,$(TARGET))

dev: CFLAGS+=-g -Wall -Wextra -O0
dev: all

all: $(TARGET) $(SO_TARGET) $(EXE_TARGET) $(AI_SOURCES) ai

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

clean:
	rm -rf build $(LIB_OBJECTS) $(EXE_OBJECTS) $(TESTS)
	rm -f tests/tests.log
	find . -name "*.gc*" -exec rm {} \;
	rm -rf `find -name "*.dSYM -print"`
