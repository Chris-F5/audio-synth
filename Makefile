CC=gcc

SRC=audiosynth.c
OBJ=$(SRC:.c=.o)

LDFLAGS=-lportaudio -lm
CFLAGS=-g

audiosynth: $(OBJ)
	$(CC) -o $@ $(OBJ) $(LDFLAGS)

.c.o: 
	$(CC) -c $< $(CFLAGS)

.PHONY=run clean

run: audiosynth
	./audiosynth

clean:
	rm -f audiosynth $(OBJ)
