CC=i686-elf-gcc
CFLAGS=-std=gnu99 -ffreestanding -O2 -Wall -Wextra -nostdlib -lgcc -fno-stack-protector
AS=nasm
ASFLAGS=-felf32
QEMU=qemu-system-x86_64

IMAGE=HypnOS.img

all: boot

boot: ./boot.s ./boot2.c
	nasm -fbin boot.s -o $(IMAGE)

	gcc -c boot2.c -o boot2.o $(CFLAGS)
	gcc boot2.o -o boot.bin $(CFLAGS) -T linker.ld

	truncate -s 4096 boot.bin
	cat boot.bin >> $(IMAGE)
	truncate -s 33553920 $(IMAGE)

	sudo mount $(IMAGE) disk -o gid=1000,uid=1000
	cp ./test.txt ./disk/test.txt
	sudo umount ./disk

clean:
	rm -rf boot.bin boot2.o $(IMAGE)

run:
	$(QEMU) -drive file=$(IMAGE),format=raw,index=0,media=disk
