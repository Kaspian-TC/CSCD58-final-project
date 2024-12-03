FLAGS = -Os
LINKED_LIBS = -lgmp -lm -lcrypto -lssl 
HELPER_FOLDER = ../shared_functions
SOURCES = router.c $(HELPER_FOLDER)/helper_func.c $(HELPER_FOLDER)/key_exchange.c $(HELPER_FOLDER)/certificates.c
OBJECTS = $(SOURCES:.c=.o)

router: $(OBJECTS)
	gcc $^ -o $@ $(FLAGS) $(LINKED_LIBS)
%.o: %.c
	gcc -c $< -o $@ $(FLAGS) $(LINKED_LIBS)
run: router
	./router

clean:
	rm router
	rm $(OBJECTS)