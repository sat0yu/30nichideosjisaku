default :
	make img

ipl.bin : ipl.nas Makefile
	nasm ipl.nas -o ipl.bin -l ipl.lst

haribote.img : ipl.bin Makefile
	mformat -f 1440 -C -B ipl.bin -i haribote.img ::

img :
	make -r haribote.img

clean :
	rm -f ipl.bin ipl.lst haribote.img
