EXE = bmtape2
CC = gcc
OBJS = main.o bm.o kansas.o srecord.o io.o
CFLAGS = -Os -finline-limit-20000 -Wall
#CFLAGS = -O0 -Wall
LDFLAGS = -s

$(EXE): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LDFLAGS)
.c.o:
	$(CC) -c $(CFLAGS) $<
src:
	rm -r -f bmtape2
	mkdir bmtape2
	cp -p main.c bm.c kansas.c srecord.c io.c srecord.h bmtape2.h Makefile bmtape2/
	nkf -w80 readme.txt | sed 's/\n//' > bmtape2/readme.txt
	tar -c bmtape2/ | gzip -9 > bmtape2src.tgz
	rm -r -f bmtape2
win32:
	rm -r -f bmtape2win32
	mkdir bmtape2win32
	cp -p bmtape2.exe bmtape2win32/
	nkf -s -Lw readme.txt > bmtape2win32/readme.txt
	zip -u -9 bmtape2win32.zip bmtape2win32/*
	rm -r -f bmtape2win32
mac:
	rm -r -f bmtape2mac
	mkdir bmtape2mac
	cp -p bmtape2 bmtape2mac/
	nkf -s readme.txt | sed 's/\r//' > bmtape2mac/readme.txt
	zip -u -9 bmtape2mac.zip bmtape2mac/*
	rm -r -f bmtape2mac
zaurus:
	rm -r -f bmtape2zaurus
	mkdir bmtape2zaurus
	cp -p bmtape2 bmtape2zaurus/
	nkf -s readme.txt | sed 's/\r//' > bmtape2zaurus/readme.txt
	zip -u -9 bmtape2zaurus.zip bmtape2zaurus/*
	rm -r -f bmtape2zaurus
clean:
	rm -f *.o
	rm -f $(EXE)
	rm -f $(EXE).exe
