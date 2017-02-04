CC = gcc
#CC = i586-mingw32msvc-gcc
CFLAGS = `pkg-config --cflags gtk+-2.0 gtkglext-1.0 gtkglext-x11-1.0`
LIBS = `pkg-config --libs gtk+-2.0 gtkglext-1.0 gtkglext-x11-1.0`

all: slider

purge: clean
	-test -f slider && rm slider
	-test -f slider.exe && rm slider.exe

clean:
	-test -f slider.o && rm slider.o

#slider.exe: slider.o
slider: slider.o
	$(CC) $^ -o $@ $(LIBS)

slider.o: slider.c
	$(CC) -c $^ -o $@ $(CFLAGS)


