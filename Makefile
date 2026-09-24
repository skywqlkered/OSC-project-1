target: main

main: main.cpp shell.cpp
	clear
	g++ main.cpp shell.cpp -o main
	

.PHONY: run


run: 
	./main

.PHONY: test

test: test.cpp shell.test.cpp
	g++ test.cpp shell.test.cpp -o test
	./test