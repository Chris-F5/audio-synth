CC=gcc

SRC=audiosynth.c linked_list.c audio.c
OBJ=$(SRC:.c=.o)

LDFLAGS=-lportaudio -lm -lSDL2 -lSDL2main -lSDL2_ttf
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
