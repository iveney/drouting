CC=/usr/bin/g++
SRC=parser.cpp header.c 
OBJ=parser.o header.o
BIN=main
DBG=debug
parser=parser
OPT=-Wall -g

.PHONY: all
all: $(OBJ) parser release debug 
	cp ./main ./util/
	ctags -R *

release:
	$(CC) $(OPT) -c route.cpp
	$(CC) $(OPT) -c $(SRC)
	$(CC) $(OPT) -o $(BIN) $(OBJ) route.o

debug:
	$(CC) $(OPT) -DDEBUG -c route.cpp
	$(CC) $(OPT) -DDEBUG -c $(SRC)
	$(CC) $(OPT) -o $(DBG) $(OBJ) route.cpp

parser:
	$(CC) $(OPT) -c parser_main.cpp
	$(CC) $(OPT) -o $(parser) $(OBJ) parser_main.o

.PHONY : clean
clean:
	rm -rf $(OBJ) $(DBG) $(BIN) $(parser)
