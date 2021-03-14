# PS IS1 321 LAB02
# Sebastian Ratańczuk
# rs44476@zut.edu.pl 

lab : lab2.o lib.o
	gcc -o lab lab2.o lib.o

lib.o : lib.c
	gcc -c lib.c

lab2.o : lab2.c
	gcc -c lab2.c	