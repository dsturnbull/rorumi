INCLUDE=-I/usr/local/lib/glib-2.0/include -I/usr/local/include/glib-2.0
LIBS=-lglib-2.0 -framework Ruby
COPTS=-Werror -Wall -pedantic -I. -g

rorumi: src/rorumi.o src/reader.o src/writer.o
	clang $(COPTS) $(INCLUDE) $(LIBS) $^ -o rorumi

src/rorumi.o: src/rorumi.c src/rorumi.h
src/reader.o: src/reader.c src/reader.h src/rorumi.h src/rorumi.c
src/writer.o: src/writer.c src/writer.h src/rorumi.h src/reader.c

src/rorumi.o src/reader.o src/writer.o:
	clang $(COPTS) $(INCLUDE) -c $< -o $(<:.c=.o)

clean:
	rm -f src/*.o
	rm -f rorumi
	rm -f rorumi.dSYM

.PHONY: clean
