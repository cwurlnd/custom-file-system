simplefs: shell.o fs.o disk.o
	gcc shell.o fs.o disk.o -o simplefs

shell.o: shell.c
	gcc -Wall shell.c -c -o shell.o -g

fs.o: fs.c fs.h
	gcc -Wall fs.c -c -o fs.o -g

disk.o: disk.c disk.h
	gcc -Wall disk.c -c -o disk.o -g

clean:
	rm -f simplefs disk.o fs.o shell.o
