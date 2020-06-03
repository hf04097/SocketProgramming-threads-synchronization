all:
	gcc  -o server.o server.c -lpthread 
	gcc  -o client.o client.c -lpthread 