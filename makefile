CFLAGS=-W -Wall
OBJECTS=BoundedBuffer.o diagnostics.o freesectordescriptorstore.o \
        generic_oneandhalfendedlist.o sectordescriptor.o \
        sectordescriptorcreator.o testharness.o fakeapplications.o \
        diskdevice.o diskdriver.o

demo: $(OBJECTS)
	gcc -o demo $(OBJECTS) -lpthread

diskdriver.o: diskdriver.c
	gcc $(CFLAGS) -c diskdriver.c

clean:
	rm -f diskdriver.o

