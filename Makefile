target: main

main: main.cpp shell.cpp
	clear
	g++ main.cpp shell.cpp -o main

.PHONY: run


run: 
	./main
