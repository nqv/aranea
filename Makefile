PKG = aranea

DEBUG       ?= 0
# Use vfork (uClinux)
VFORK       ?= 1
# Support CGI
CGI         ?= 1

SRC = aranea.c server.c client.c http.c mimetype.c cgi.c
OBJ = ${SRC:.c=.o}

CFLAGS_DEBUG = -Werror -O0 -g -DDEBUG
CFLAGS_NDEBUG = -DNDEBUG

CFLAGS += -Wall -Wextra

ifeq (${DEBUG},1)
CFLAGS += ${CFLAGS_DEBUG}
else
CFLAGS += ${CFLAGS_NDEBUG}
endif
ifeq (${VFORK},1)
CFLAGS += -DHAVE_VFORK=1
endif
ifeq (${CGI},1)
CFLAGS += -DHAVE_CGI=1
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
