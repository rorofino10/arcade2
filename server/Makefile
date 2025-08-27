CC = gcc
CFLAGS = -g -IC:/raylib/include -I../shared/include -Isrc
LDFLAGS =  -LC:/raylib/lib
LDLIBS = -lws2_32 -lraylib -lopengl32 -lgdi32 -lwinmm

CODEDIRS = src
CFILES = $(foreach D,$(CODEDIRS),$(wildcard $(D)/*.c))
OBJECTS = $(patsubst src/%,build/%,$(CFILES:.c=.o))

BINDIR = bin
BUILDDIR = build
BINNAME = server
BINARY = ${BINDIR}/${BINNAME}.exe

all: ${BINARY}

${BINARY}: ${OBJECTS} | ${BINDIR}
	${CC} -o $@ $^ ${LDFLAGS} ${LDLIBS}

build/%.o: src/%.c | ${BUILDDIR}
	mkdir -p $(dir $@)
	${CC} ${CFLAGS} -c $< -o $@

${BUILDDIR}:
	mkdir -p ${BUILDDIR}

${BINDIR}:
	mkdir -p ${BINDIR}

test:
	./${BINARY}

clean:
	rm -rf ${BUILDDIR} ${BINDIR}
