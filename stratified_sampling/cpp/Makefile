CC=g++
CFLAGS=-I.
LIBS=

%.o: %.cpp
	$(CC) -c -o $@ $< $(CFLAGS)

strat_samp: strat_samp.o
	gcc -o strat_samp strat_samp.o -I. `pkg-config --cflags --libs opencv`

.PHONY: clean

clean:
	rm -f *.o strat_samp
