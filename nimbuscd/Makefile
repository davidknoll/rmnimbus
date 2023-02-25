CFLAGS=/G1 /AT /Gs /Zp /nologo
CC=qcl

all: cdtest.exe inquiry.exe nimbuscd.sys

clean:
	del *.obj
	del *.exe
	del *.sys
	del *.map

cdtest.exe: cdtest.obj driver.obj sasicmd.obj cdriver.obj
	qlink /nologo cdtest.obj driver.obj sasicmd.obj cdriver.obj, cdtest.exe,,,

inquiry.exe: inquiry.obj sasicmd.obj
	qlink /nologo inquiry.obj sasicmd.obj, inquiry.exe,,,

nimbuscd.sys: driver.obj sasicmd.obj cdriver.obj
	qlink /nologo driver.obj sasicmd.obj cdriver.obj, nimbuscd.exe, nimbuscd.map,,
	exe2bin nimbuscd.exe nimbuscd.sys

driver.obj: driver.asm
	$(CC) $(CFLAGS) -c driver.asm
