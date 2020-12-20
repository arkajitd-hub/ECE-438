EXEC_CSMA = csma
COMPILER = g++ -std=c++11
all: csma
csma: csma.o node.o
	$(COMPILER) -pthread csma.o node.o -o csma
csma.o: csma.cpp
	$(COMPILER) -c csma.cpp
node.o: node.cpp
	$(COMPILER) -c node.cpp
clean:
	-rm *.o
