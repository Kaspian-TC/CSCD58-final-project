client: client.c
	gcc -o client client.c -lgmp -lm
run: client
	./client server
clean:
	rm client
	rm *.o

gmp:
	./install_gmp.sh