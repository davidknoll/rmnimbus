WATCOM=/opt/openwatcom

all: xr.com multi.sys

cp: all
	mcopy -o -i "/Volumes/GOTEK/Nimbus 186/Startup-Welcome/System.img" xr.com ::
	mcopy -o -i "/Volumes/GOTEK/Nimbus 186/Startup-Welcome/System.img" multi.sys ::
	diskutil eject /Volumes/GOTEK

clean:
	rm -f *~
	rm -f xr.com xr.o
	rm -f multi.sys multi.lst

xr.com: xr.c
	INCLUDE=$(WATCOM)/h wcl -mt $<

multi.sys: multi.asm
	nasm -l multi.lst -o $@ $<
