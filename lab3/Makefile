# PS IS1 321 LAB03
# Sebastian Ratańczuk
# rs44476@zut.edu.pl 

lab : libtest.a lab1.c
	gcc lab1.c -L. -ltest

libtest.a : lib.o
	ar cr libtest.a lib.o

lib.o : lib.c
	gcc -c lib.c