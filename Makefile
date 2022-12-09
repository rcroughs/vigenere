PROG = vigenere kasiski
OBJS = ${PROG:=.o}
SRCS = ${PROG:=.c}

all: ${PROG}
vigenere: vigenere.o
kasiski:  kasiski.o

.o:
	${CC} ${LDFLAGS} -lm -o $@ $<

.c.o:
	${CC} -std=c99 -pedantic -D_POSIX_C_SOURCE=200809L ${CFLAGS} ${CPPFLAGS} -c $<

README: vigenere.6
	man -l vigenere.6 | sed 's/.//g' >README

tags: ${SRCS}
	ctags ${SRCS}

clean:
	rm -f ${OBJS} ${PROG} ${PROG:=.core} tags

.PHONY: all tags clean
