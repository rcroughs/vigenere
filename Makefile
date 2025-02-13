PROG = kasiski encode
OBJS = ${PROG:=.o}
SRCS = ${PROG:=.c}

all: ${PROG}
kasiski:  kasiski.o

.o:
	${CC} ${LDFLAGS} -lm -o $@ $<

.c.o:
	${CC} -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L ${CFLAGS} ${CPPFLAGS} -c $<

clean:
	rm -f ${OBJS} ${PROG} ${PROG:=.core} tags

.PHONY: all
