# Aranea

include config.mk

SRC = src/aranea.c \
	src/server.c \
	src/state.c \
	src/client.c \
	src/clientpool.c \
	src/http.c \
	src/mimetype.c 

CFLAGS += -Wall -Wextra --std=gnu99 -I./include
CFLAGS_DEBUG = -Werror -O0 -g -DDEBUG
CFLAGS_NDEBUG = -DNDEBUG

CFLAGS += -DHAVE_VFORK=${VFORK} -DHAVE_CGI=${CGI} -DHAVE_CHROOT=${CHROOT}
CFLAGS += -DHAVE_AUTH=${AUTH}

ifeq (${DEBUG},1)
CFLAGS += ${CFLAGS_DEBUG}
else
CFLAGS += ${CFLAGS_NDEBUG}
endif

ifeq (${CGI},1)
SRC += src/cgi.c
endif

ifeq (${AUTH},1)
SRC += src/auth.c \
	src/util.c
endif

OBJ = ${SRC:.c=.o}

all: options ${PKG}

${PKG}: ${OBJ}
	@echo CC -o $@
	@${CC} ${CFLAGS} ${LIBS} -o $@ ${OBJ} 

options:
	@echo ${PGK} build options:
	@echo "CFLAGS  = ${CFLAGS}"
	@echo "LIBS    = ${LIBS}"
	@echo "CC      = ${CC}"

%.o: %.c
	@echo CC $<
	@${CC} -c ${CFLAGS} -o $@ $<

clean:
	@rm -rf ${PKG} src/*.o
