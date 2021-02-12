CC = gcc -Wpedantic -std=gnu99 
myShell: assignment1.c
	$(CC) -o myShell assignment1.c
clean: 
	rm myShell 