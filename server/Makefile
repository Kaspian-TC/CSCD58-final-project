FLAGS = -Os
LINKED_LIBS = -lgmp -lm

server: server.c
	gcc -o server server.c $(FLAGS) $(LINKED_LIBS)

run: server
	./server

clean:
	rm -f server
