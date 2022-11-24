CC=gcc

NAME=vcpu816-al-as

VER_MAJOR=0
VER_MINOR=1
VER_PATCH=0

DEFINES=VERSION_STR=\"$(VER_MAJOR).$(VER_MINOR).$(VER_PATCH)\"

COMMON_FLAGS=-std=c11 -Wall -Wpedantic
C_FLAGS=$(COMMON_FLAGS) $(addprefix -D, $(DEFINES)) -c
L_FLAGS=$(COMMON_FLAGS)

TARGET=debug

SRC=$(wildcard src/*.c)
OBJ=$(SRC:src/%.c=target/$(TARGET)/obj/%.o)
OUTPUT=target/$(TARGET)/$(NAME)

all: $(TARGET)

debug: COMMON_FLAGS+=-g -O0
debug: DEFINES+=DEBUG
debug: $(OUTPUT)

release: COMMON_FLAGS+=-O2
release: DEFINES+=RELEASE
release: $(OUTPUT)

$(OUTPUT): $(OBJ)
	@mkdir -p $(@D)
	$(CC) $(L_FLAGS) -o $@ $^

target/$(TARGET)/obj/%.o: src/%.c $(wildcard src/*.h)
	@mkdir -p $(@D)
	$(CC) $(C_FLAGS) -o $@ $<

clean:
	rm -r target 2> /dev/null || true

.PHONY: all debug release clean
