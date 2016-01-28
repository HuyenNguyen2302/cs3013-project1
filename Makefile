all: runCommand shell shell2

runCommand: runCommand.o
	gcc -o runCommand runCommand.o

runCommand.o: runCommand.c
	gcc -c runCommand.c

shell: shell.o
	gcc -o shell shell.o

shell.o: shell.c
	gcc -c shell.c -std=gnu99

shell2: shell2.o
	gcc -o shell2 shell2.o

shell2.o: shell2.c
	gcc -c shell2.c -std=gnu99

clean:
	rm -rf *.o runCommand shell shell2