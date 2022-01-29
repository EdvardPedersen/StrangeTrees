
main: main.c plot.c
	gcc -g main.c plot.c -o main plot.h -lSDL2 -lm 

run: main
	sudo ./main

drawTree: main
	sudo ./main -d

animatedSearch: main
	sudo ./main -a

drawAnimated: main
	sudo ./main -da

.PHONY: clean

clean:
	rm -rf main tree*