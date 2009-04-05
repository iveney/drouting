CC=/usr/bin/g++
SRC=parser.cpp header.cpp GridPoint.cpp Router.cpp util.cpp 
OBJ=$(SRC:.cpp=.o)
BIN=main
DBG=debug
parser=parser
OPT=-Wall -g

all: $(OBJ) parser release debug
	cp ./main ./util/
	ctags -R *.cpp *.h

release: $(OBJ)
	$(CC) $(OPT) -c main.cpp
	$(CC) $(OPT) -o $(BIN) $(OBJ) main.o

debug: $(OBJ)
	$(CC) -c -DDEBUG $(SRC) main.cpp
	$(CC) $(OPT) -o $(DBG) $(OBJ) main.o

parser: $(OBJ)
	$(CC) $(OPT) -c parser_main.cpp
	$(CC) $(OPT) -o $(parser) $(OBJ) parser_main.o

%.o: %.cpp 
	$(CC) -c $< $(OPT) -o $@

tags: $(SRC)
	ctags -R *.cpp *.h

.PHONY : clean
clean:
	rm -rf $(OBJ) $(DBG) $(BIN) $(parser)
