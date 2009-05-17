CC=g++
SRC=parser.cpp header.cpp GridPoint.cpp Router.cpp util.cpp \
    ConstraintGraph.cpp draw_voltage.cpp
HDR=$(SRC:.cpp=.h)
OBJ=$(SRC:.cpp=.o)
BIN=main
DBG=debug
PARSER=parser
OPT=-Wall #-g 

main: $(OBJ) main.o tags
	@echo "Making main..."
	$(CC) $(OPT) -o $(BIN) $(OBJ) main.o

parser: $(OBJ) parser_main.o
	@echo "Making parser..."
	$(CC) $(OPT) -o $(PARSER) $(OBJ) parser_main.o

all: parser main debug
	cp ./main ./util/

debug: $(OBJ) main.cpp
	@echo "Making debug..."
	$(CC) -c -DDEBUG $(SRC) main.cpp
	$(CC) $(OPT) -o $(DBG) $(OBJ) main.o

%.o: %.cpp  %.h
	$(CC) -c $< $(OPT) -o $@

tags: $(SRC) $(HDR) main.cpp main.h parser_main.cpp
	@echo "Making tags..."
	cscopegen

.PHONY : clean
clean:
	@echo "Cleaning all..."
	rm -rf *.o $(OBJ) $(DBG) $(BIN) $(parser)
