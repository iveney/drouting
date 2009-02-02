CC=/usr/bin/g++
SRC=parser.cpp route.cpp header.c
OBJ=parser.o route.o header.o
BIN=main
DBG=debug
OPT=-Wall -g
default: DEBUG RELEASE 
	ctags -R *

RELEASE:
	$(CC) $(OPT) -c $(SRC)
	$(CC) $(OPT) -o $(BIN) $(OBJ)

DEBUG:
	$(CC) $(OPT) -DDEBUG -c $(SRC)
	$(CC) $(OPT) -o $(DBG) $(OBJ)

.PHONY : clean
clean:
	rm -rf $(OBJ) $(DBG) $(BIN)
