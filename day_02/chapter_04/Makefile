default :
	make img

ipl.bin : ipl.nas Makefile
	nasm ipl.nas -o ipl.bin -l ipl.lst

helloos.img : ipl.bin Makefile
	mformat -f 1440 -C -B ipl.bin -i helloos.img ::

img :
	make -r helloos.img

clean :
	rm -f ipl.bin ipl.lst helloos.img
