FLAGS = -Os
LINKED_LIBS = -lgmp -lm
HELPER_FOLDER = ../shared_functions
SOURCES = client.c $(HELPER_FOLDER)/helper_func.c
OBJECTS = $(SOURCES:.c=.o)

client: $(OBJECTS)
	gcc $^ -o $@ $(FLAGS) $(LINKED_LIBS)
%.o: %.c
	gcc -c $< -o $@ $(FLAGS) $(LINKED_LIBS)
run: client
	./client server
clean:
	rm client
	rm $(OBJECTS)