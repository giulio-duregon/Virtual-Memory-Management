CXX=g++
CXXFLAGS=-g -std=c++11 -Wall -pedantic -lstdc++ -Wvariadic-macros
BIN=des_mmu

SRC=$(wildcard *.cpp)
OBJ=$(SRC:%.cpp=%.o)

all: $(OBJ)
	$(CXX) -o $(BIN) $^

%.o: %.c
	$(CXX) $@ -c $<

	
run: all
	./$(BIN) -v -t -e -p -s F input1 rfile

clean:
	rm -f *.o
	rm $(BIN)