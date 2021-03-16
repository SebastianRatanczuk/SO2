# PS IS1 321 LAB03
# Sebastian Ratańczuk
# rs44476@zut.edu.pl 

lab.out : libtest.a rs44476.ps.lab03.static.main.c
	gcc rs44476.ps.lab03.static.main.c -o lab.out -L. -ltest

libtest.a : rs44476.ps.lab03.static.lib.o
	ar cr libtest.a rs44476.ps.lab03.static.lib.o

rs44476.ps.lab03.static.lib.o : rs44476.ps.lab03.static.lib.c
	gcc -c rs44476.ps.lab03.static.lib.c