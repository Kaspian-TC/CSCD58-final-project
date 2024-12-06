FLAGS = -Os
LINKED_LIBS = -lgmp -lm -lcrypto -lssl 
HELPER_FOLDER = ../shared_functions
SOURCES = server.c $(HELPER_FOLDER)/helper_func.c $(HELPER_FOLDER)/key_exchange.c $(HELPER_FOLDER)/certificates.c
OBJECTS = $(SOURCES:.c=.o)

server: $(OBJECTS)
	gcc $^ -o $@ $(FLAGS) $(LINKED_LIBS)
%.o: %.c
	gcc -c $< -o $@ $(FLAGS) $(LINKED_LIBS)
run: server
	./server

clean:
	rm server
	rm $(OBJECTS)