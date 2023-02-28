all:
	g++ -w -std=gnu++0x -g manager.cpp -o sniffer
	g++ -std=gnu++0x worker.cpp -o worker
clean:
	rm sniffer
	rm worker
	rm output/*
