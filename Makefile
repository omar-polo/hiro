CC =		cc
CFLAGS =	-O0 -pipe -g -Wall -Wextra \
		-Wmissing-prototypes -Wstrict-prototypes \
		-Wwrite-strings -Wno-unused-parameter \
		-I.
LDFLAGS =	-levent

.PHONY: all clean TAGS

all: hirod hiroctl TAGS compile_flags.txt

# TODO: replace this with a proper depend stuff
hirod.o hiroctl.o: hiro.h
hirod.o hiroctl.o cmd.o: cmd.h
hirod.o hiroctl.o util.o: util.h
hirod.o log.o can.o: log.h
hirod.o can.o: can.h

hirod: hirod.o cmd.o util.o log.o can.o
	${CC} hirod.o cmd.o util.o log.o can.o -o $@ ${LDFLAGS}

hiroctl: hiroctl.o cmd.o util.o
	${CC} hiroctl.o cmd.o util.o -o $@ ${LDFLAGS}

TAGS:
	-etags *.c || true

compile_flags.txt:
	printf "%s\n" ${CFLAGS} > compile_flags.txt

clean:
	rm -f *.o hirod hiroctl
	rm -f TAGS compile_flags.txt
