CC=g++
SRC=parser.cpp header.cpp GridPoint.cpp Router.cpp util.cpp \
    ConstraintGraph.cpp draw_voltage.cpp
HDR=$(SRC:.cpp=.h)
OBJ=$(SRC:.cpp=.o)
BIN=main
DBG=debug
parser=parser
OPT=-Wall -g #-DOUTPUT

release: $(OBJ) main.o tags
	@echo "Making release..."
	$(CC) -c main.cpp
	$(CC) $(OPT) -o $(BIN) $(OBJ) main.o

all: parser release debug
	cp ./main ./util/

debug: $(HDR) $(SRC)
	@echo "Making debug..."
	$(CC) -c -DDEBUG $(SRC) main.cpp
	$(CC) $(OPT) -o $(DBG) $(OBJ) 

parser: $(OBJ) parser_main.o
	@echo "Making parser..."
	$(CC) $(OPT) -o $(parser) $(OBJ) parser_main.o

%.o: %.cpp  %.h
	$(CC) -c $< $(OPT) -o $@

tags: $(SRC) $(HDR)
	@echo "Making tags..."
	cscopegen

.PHONY : clean
clean:
	@echo "Cleaning all..."
	rm -rf $(OBJ) $(DBG) $(BIN) $(parser)
