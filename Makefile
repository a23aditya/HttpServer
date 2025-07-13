
run:
	@gcc -o server main.c server.c
	./server

all:
	gcc -o server main.c server.c
	
clean:
	@rm server
