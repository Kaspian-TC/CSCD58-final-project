FLAGS = -Os
LINKED_LIBS = -lgmp -lm

router: router.c
	gcc -o router router.c $(FLAGS) $(LINKED_LIBS)

run: router
	./router

clean:
	rm -f router
