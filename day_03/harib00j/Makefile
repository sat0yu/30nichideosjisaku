default :
	make img

ipl10.bin : ipl10.nas Makefile
	nasm ipl10.nas -o ipl10.bin -l ipl10.lst

asmhead.bin : asmhead.nas Makefile
	nasm asmhead.nas -o asmhead.bin -l asmhead.lst

naskfunc.o : naskfunc.nas Makefile
	nasm -g -f elf naskfunc.nas -o naskfunc.o -l naskfunc.lst

bootpack.hrb : bootpack.c naskfunc.o hrb.ld Makefile
	gcc -fno-pie -no-pie -march=i486 -m32 -nostdlib -T hrb.ld bootpack.c naskfunc.o -o bootpack.hrb

haribote.sys : asmhead.bin bootpack.hrb Makefile
	cat asmhead.bin bootpack.hrb > haribote.sys

haribote.img : ipl10.bin haribote.sys Makefile
	mformat -f 1440 -C -B ipl10.bin -i haribote.img ::
	mcopy -i haribote.img haribote.sys ::

img :
	make -r haribote.img

.PHONY: docker-build
docker-build:
	docker build -t haribos .

.PHONY: docker-make
docker-make:
	docker run -it --rm -v $(CURDIR):/haribos haribos make img

.PHONY: run
run :
	make docker-make
	qemu-system-i386 -drive file=haribote.img,format=raw,if=floppy -boot a

clean :
	rm -f *.bin *.lst *.sys *.img *.o *.hrb
