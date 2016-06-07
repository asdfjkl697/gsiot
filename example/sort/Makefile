objects=sort.o mytool1.o mytool2.o
sort:$(objects)
	gcc -o $@ $^
sort.o:sort.c mytool1.h mytool2.h
	gcc -c $<
mytool1.o:mytool1.c mytool1.h
	gcc -c $<
mytool2.o:mytool2.c mytool2.h
	gcc -c $<
clean:
	-rm main $(objects)
