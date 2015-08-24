CFLAGS=-O2 -Isrc -rdynamic $(OPTFLAGS)
LDLIBS=-ldl -lm $(OPTLIBS)
PREFIX?=/usr/local

SOURCES=$(wildcard src/**/*.c)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

EXE_SOURCES=$(wildcard src/*.c)
EXE_OBJECTS=$(patsubst %.c,%.o,$(EXE_SOURCES))

EXE_TARGET=build/cellhack

TEST_SRC=$(wildcard tests/*_tests.c)
TESTS=$(patsubst %.c,%,$(TEST_SRC))

TARGET=build/libcellhack.a
SO_TARGET=$(patsubst %.a,%.so,$(TARGET))

# The Target build
dev: CFLAGS=-g -Wall -Wextra $(OPTFLAGS)
dev: all

all: $(TARGET) $(SO_TARGET) tests

$(TARGET): CFLAGS += -fPIC
$(TARGET): build $(OBJECTS) $(EXE_OBJECTS)
	ar rcs $@ $(OBJECTS)
	ranlib $@
	$(CC) $(CFLAGS) -o $(EXE_TARGET) $(EXE_OBJECTS) $(TARGET) $(LDLIBS)

$(SO_TARGET): $(TARGET) $(OBJECTS)
	$(CC) --shared -o $@ $(OBJECTS)

build:
	@mkdir -p build
	@mkdir -p bin

# The Unit tests
.PHONY: tests
tests: CFLAGS += $(TARGET)
tests: $(TESTS)
	sh ./tests/runtests.sh

valgrind:
	VALGRIND="valgrind --log-file=/tmp/valgrind-%p.log" $(MAKE)

# The Cleaner
clean:
	rm -rf build $(OBJECTS) $(TESTS) $(EXE_OBJECTS)
	rm -f tests/tests.log
	find . -name "*.gc*" -exec rm {} \;
	rm -rf `find -name "*.dSYM -print"`

# The Install
install: all
	install -d $(DESTDIR)/$(PREFIX)/lib/
	install $(TARGET) $(DESTDIR)/$(PREFIX)/lib/

# The Checker
BADFUNCS='[^_.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)|stpn?cpy|a?sn?printf|byte_)'
check:
	@echo Files with potentially dangerous functions.
	@egrep $(BADFUNCS) $(SOURCES) || true
