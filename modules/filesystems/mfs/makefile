# Makefile for building a module dll that can be loaded by MollenOS
# Valid for drivers
SOURCES = $(wildcard ./*.c)

INCLUDES = -I../../../librt/include
OBJECTS = $(SOURCES:.c=.o)

CFLAGS = $(GCFLAGS) -D_DLL $(INCLUDES)
LFLAGS = /nodefaultlib /subsystem:native /noentry /dll ../../../librt/build/libc.lib ../../../librt/build/libdrv.lib ../../../librt/build/libos.lib

.PHONY: all
all: ../../build/mfs.dll ../../build/mfs.mdrv

../../build/mfs.dll: $(OBJECTS)
	$(LD) $(LFLAGS) $(OBJECTS) /out:$@

../../build/mfs.mdrv: mfs.mdrv
	cp $< $@

%.o : %.c
	$(CC) -c $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	rm -f ../../build/mfs.dll
	rm -f ../../build/mfs.lib
	rm -f ../../build/mfs.mdrv
	rm -f $(OBJECTS)