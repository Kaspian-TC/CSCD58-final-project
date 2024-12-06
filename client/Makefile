FLAGS = -Os
LINKED_LIBS = -lgmp -lm -lcrypto -lssl 
HELPER_FOLDER = ../shared_functions
SOURCES = client.c $(HELPER_FOLDER)/helper_func.c $(HELPER_FOLDER)/key_exchange.c $(HELPER_FOLDER)/certificates.c
OBJECTS = $(SOURCES:.c=.o)

client: $(OBJECTS)
	gcc $^ -o $@ $(FLAGS) $(LINKED_LIBS)
%.o: %.c
	gcc -c $< -o $@ $(FLAGS) $(LINKED_LIBS)
retrieve: client
	./client --retrieve 10.0.0.2
store: client
	./client --store 10.0.0.2

clean:
	rm client
	rm $(OBJECTS)