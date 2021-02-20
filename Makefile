CC =		cc
CFLAGS =	-O2 -pipe -g -Wall -Wextra \
		-Wmissing-prototypes -Wstrict-prototypes \
		-Wwrite-strings -Wno-unused-parameter \
		-Icompat
LDFLAGS =	-levent

.PHONY: all clean TAGS

all: hirod hiroctl TAGS compile_flags.txt

# TODO: replace this with a proper depend stuff
hirod.o hiroctl.o: hiro.h
hirod.o hiroctl.o cmd.o: cmd.h
hirod.o hiroctl.o util.o: util.h
hirod.o log.o: log.h

hirod: hirod.o cmd.o util.o log.o
	${CC} hirod.o cmd.o util.o log.o -o $@ ${LDFLAGS}

hiroctl: hiroctl.o cmd.o util.o
	${CC} hiroctl.o cmd.o util.o -o $@ ${LDFLAGS}

TAGS:
	-etags *.c || true

compile_flags.txt:
	printf "%s\n" ${CFLAGS} > compile_flags.txt

clean:
	rm -f *.o hirod hiroctl
	rm -f TAGS compile_flags.txt
