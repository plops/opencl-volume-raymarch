CFLAGS=-ggdb -Wall -Wextra
LDFLAGS=-lOpenCL -lglfw -lm
all: ocl-test
%:
ocl-test:
kopie: 
	cp ocl-test.c $(CHK_SOURCES)

check-syntax: kopie $(CHK_SOURCES:.c=.o)
