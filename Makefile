CC=/usr/bin/g++
SRC=parser.cpp header.cpp GridPoint.cpp Router.cpp util.cpp ConstraintGraph.cpp
HDR=$(SRC:.cpp=.h)
OBJ=$(SRC:.cpp=.o)
BIN=main
DBG=debug
parser=parser
OPT=-Wall -g

release: $(OBJ) tags
	echo "Making release..."
#$(CC) $(OPT) -c $(SRC) main.cpp
	$(CC) $(OPT) -o $(BIN) $(OBJ) main.o

all: parser release debug
	cp ./main ./util/

debug: $(HDR) $(SRC)
	echo "Making debug..."
	$(CC) -c -DDEBUG $(SRC) main.cpp
	$(CC) $(OPT) -o $(DBG) $(OBJ) main.o

parser: parser_main.cpp
	echo "Making parser..."
#$(CC) $(OPT) -c parser_main.cpp
	$(CC) $(OPT) -o $(parser) $(OBJ) parser_main.o

%.o: %.cpp  %.h
	$(CC) -c $< $(OPT) -o $@

tags: $(SRC) $(HDR)
	cscopegen

.PHONY : clean
clean:
	rm -rf $(OBJ) $(DBG) $(BIN) $(parser)
