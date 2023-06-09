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
	./$(BIN) -f 4 -a e -o aOPSF in1 rfile

clean:
	rm -f *.o
	rm $(BIN)