CC=/usr/bin/g++
SRC=parser.cpp route.cpp header.c
OBJ=parser.o route.o header.o
BIN=main
DBG=debug
OPT=-Wall -g
all:release debug
	ctags -R *
release:
	$(CC) $(OPT) -c $(SRC)
	$(CC) $(OPT) -o $(BIN) $(OBJ)
debug:
	$(CC) $(OPT) -DDEBUG -c $(SRC)
	$(CC) $(OPT) -o $(DBG) $(OBJ)
clean:
	rm -rf $(OBJ) $(BIN)
