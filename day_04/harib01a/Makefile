OSNAME := haribote
OSBUILDER := ${OSNAME}-builder

%.bin : %.nas
	nasm $^ -o $@ -l $*.lst

%.o : %.nas
	nasm -g -f elf $^ -o $@ -l $*.lst

BOOTPACK_FILES := bootpack.c naskfunc.o
bootpack.hrb : ${BOOTPACK_FILES} hrb.ld Makefile
	gcc -fno-pie -no-pie -march=i486 -m32 -nostdlib -T hrb.ld -o $@ -g -O0 ${BOOTPACK_FILES}

${OSNAME}.sys : asmhead.bin bootpack.hrb
	cat $^ > $@

${OSNAME}.img : ipl10.bin ${OSNAME}.sys
	mformat -f 1440 -C -B ipl10.bin -i $@ ::
	mcopy -i $@ ${OSNAME}.sys ::

img :
	make ${OSNAME}.img

.PHONY: docker-build
docker-build:
	docker build -t ${OSBUILDER} .

.PHONY: docker-make
docker-make:
	docker run -it --rm -v $(CURDIR):/${OSNAME} -w /${OSNAME} ${OSBUILDER} make img

.PHONY: run
run :
	make docker-build
	make docker-make
	qemu-system-i386 -L . -m 32 -rtc base=localtime -vga std -drive file=${OSNAME}.img,index=0,if=floppy,format=raw

clean :
	rm -f *.bin *.lst *.sys *.img *.o *.hrb
