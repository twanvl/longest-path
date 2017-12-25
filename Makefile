BLOSSOM=blossom5-v2.05.src

all: longest-path

longest-path: longest-path.cpp
	make -C $(BLOSSOM) PM*.o MinCost/MinCost.o
	g++ -Wall $^ $(BLOSSOM)/PM*.o $(BLOSSOM)/MinCost/MinCost.o -std=c++11 -o $@

