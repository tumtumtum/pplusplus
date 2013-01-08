CC=gcc -m32

all: p++

install: all
	cp p++ /usr/local/bin

compiler.o: compiler.c
	CC -c compiler.c

expressions.o: expressions.c
	CC -c expressions.c

hashtable.o: hashtable.c
	CC -c hashtable.c

interpretor.o: interpretor.c
	CC -c interpretor.c

p++.o: p++.c
	CC -c p++.c

p++: compiler.o expressions.o hashtable.o interpretor.o p++.o
	CC compiler.o expressions.o hashtable.o interpretor.o p++.o -o p++ -lncurses

clean:
	rm -rf *.o p++
