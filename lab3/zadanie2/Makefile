# PS IS1 321 LAB03
# Sebastian Ratańczuk
# rs44476@zut.edu.pl 

lab2.out : libtest.so.0.1
	gcc lab1.c -ldl -o lab2.out

libtest.so.0.1 : libtest.o
	gcc -shared -nostartfiles -fPIC libtest.o -o libtest.so.0.1

libtest.o : libtest.c
	gcc -c -fPIC libtest.c