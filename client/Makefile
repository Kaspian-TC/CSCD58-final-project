FLAGS = -Os
LINKED_LIBS = -lgmp -lm
client: client.c
	gcc -o client client.c $(FLAGS) $(LINKED_LIBS)
run: client
	./client server
clean:
	rm client