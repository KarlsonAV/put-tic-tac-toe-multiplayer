CXX = g++
CXXFLAGS = -Wall -pthread -std=c++11 

server: server.cpp
	$(CXX) $(CXXFLAGS) -o server server.cpp

.PHONY: clean run

clean:
	rm -f server

run: server
	./server
