default :
	make img

ipl10.bin : ipl10.nas Makefile
	nasm ipl10.nas -o ipl10.bin -l ipl10.lst

haribote.sys : haribote.nas Makefile
	nasm haribote.nas -o haribote.sys -l haribote.lst

haribote.img : ipl10.bin haribote.sys Makefile
	mformat -f 1440 -C -B ipl10.bin -i haribote.img ::
	mcopy -i haribote.img haribote.sys ::

img :
	make -r haribote.img

clean :
	rm -f ipl10.bin ipl10.lst haribote.sys haribote.lst haribote.img
