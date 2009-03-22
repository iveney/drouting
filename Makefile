CC=/usr/bin/g++
SRC=parser.cpp header.c route.cpp
OBJ=parser.o header.o route.o
BIN=main
DBG=debug
parser=parser
OPT=-Wall -g

.PHONY: all
all: $(OBJ) parser release debug 
	cp ./main ./util/
	ctags -R *.c *.cpp *.h

release:
	$(CC) $(OPT) -c $(SRC)
	$(CC) $(OPT) -c main.cpp
	$(CC) $(OPT) -o $(BIN) $(OBJ) main.o

debug:
	$(CC) $(OPT) -DDEBUG -c $(SRC)
	$(CC) $(OPT) -DDEBUG -c main.cpp
	$(CC) $(OPT) -o $(DBG) $(OBJ) main.o

parser:
	$(CC) $(OPT) -c parser_main.cpp
	$(CC) $(OPT) -o $(parser) $(OBJ) parser_main.o

%.o: %.cpp 
	$(CC) -c $< $(OPT) -o $@

.PHONY : clean
clean:
	rm -rf $(OBJ) $(DBG) $(BIN) $(parser)
