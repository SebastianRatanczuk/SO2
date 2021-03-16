# PS IS1 321 LAB03
# Sebastian Ratańczuk
# rs44476@zut.edu.pl 

rs44476.ps.lab03.shared.main.out : rs44476.ps.lab03.shared.lib.so.0.1
	gcc rs44476.ps.lab03.shared.main.c -ldl -o rs44476.ps.lab03.shared.main.out

rs44476.ps.lab03.shared.lib.so.0.1 : rs44476.ps.lab03.shared.lib.o
	gcc -shared -nostartfiles -fPIC rs44476.ps.lab03.shared.lib.o -o rs44476.ps.lab03.shared.lib.so.0.1

rs44476.ps.lab03.shared.lib.o : rs44476.ps.lab03.shared.lib.c
	gcc -c -fPIC rs44476.ps.lab03.shared.lib.c