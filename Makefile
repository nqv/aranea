PKG = aranea
DEBUG ?= 0

SRC = aranea.c server.c client.c http.c
OBJ = ${SRC:.c=.o}

CFLAGS_DEBUG = -Werror -O0 -g -DDEBUG
CFLAGS_NDEBUG = -DNDEBUG

CFLAGS += -Wall -Wextra

ifeq (${DEBUG},1)
CFLAGS += ${CFLAGS_DEBUG}
else
CFLAGS += ${CFLAGS_NDEBUG}
endif

all: options ${PKG}

${PKG}: ${OBJ}
	@echo CC -o $@
	@${CC} ${CFLAGS} ${LIBS} -o $@ ${OBJ} 

options:
	@echo ${PGK} build options:
	@echo "CFLAGS  = ${CFLAGS}"
	@echo "LDFLAGS = ${LDFLAGS}"
	@echo "LIBS    = ${LIBS}"
	@echo "DFLAGS  = ${DFLAGS}"
	@echo "CC      = ${CC}"

%.o: %.c
	@echo CC $<
	@${CC} -c ${CFLAGS} -o $@ $<

clean:
	@rm -rf ${PKG} ${OBJ}