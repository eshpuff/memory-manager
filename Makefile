CC=gcc
CFLAGS=-Wall -Iinclude -g
SRC=src
BIN=bin

OBJS=$(SRC)/main.o $(SRC)/hash.o $(SRC)/utils.o $(SRC)/optimal.o

all: $(BIN)/main.exe

$(BIN)/main.exe: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

$(SRC)/main.o: $(SRC)/main.c include/simulator.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(SRC)/hash.o: $(SRC)/hash.c include/simulator.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(SRC)/utils.o: $(SRC)/utils.c include/simulator.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(SRC)/optimal.o: $(SRC)/optimal.c include/simulator.h
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
		rm -f *.o */*.o simulador simuladorotimo leitor-acessos

run:
	./$(BIN)/main.exe
