PKG = aranea

DEBUG       ?= 0
# Use vfork (uClinux)
VFORK       ?= 0
# Support CGI
CGI         ?= 0
# Use chroot
CHROOT      ?= 0
# Authorization
AUTH        ?= 0

SRC = src/aranea.c \
	src/server.c \
	src/client.c \
	src/http.c \
	src/mimetype.c 

CFLAGS += -Wall -Wextra --std=gnu99 -I./include
CFLAGS_DEBUG = -Werror -O0 -g -DDEBUG
CFLAGS_NDEBUG = -DNDEBUG

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
SRC += src/cgi.c
endif
ifeq (${CHROOT},1)
CFLAGS += -DHAVE_CHROOT=1
endif
ifeq (${AUTH},1)
CFLAGS += -DHAVE_AUTH=1
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
	@echo "LDFLAGS = ${LDFLAGS}"
	@echo "LIBS    = ${LIBS}"
	@echo "DFLAGS  = ${DFLAGS}"
	@echo "CC      = ${CC}"

%.o: %.c
	@echo CC $<
	@${CC} -c ${CFLAGS} -o $@ $<

clean:
	@rm -rf ${PKG} ${OBJ}
