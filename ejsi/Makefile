ejsi: ejsi.c
	gcc -O2 -o ejsi ejsi.c

ejsi-64: ejsi.c
	gcc -O2 -o ejsi ejsi.c -DBIT_64=1

ejsi-32: ejsi.c
	gcc -m32 -O2 -o ejsi ejsi.c -DBIT_32=1

clean:
	rm -f *.o
	rm -f ejsi
