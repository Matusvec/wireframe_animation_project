#Matus Vecera
#Makefile for Lab 11 mini-final project
CC = gcc
CFLAGS = -Wall -std=c99 -O3 -ffast-math
LIBS = -lX11 -lm

project: project.o gfx.o
	$(CC) -o project project.o gfx.o $(LIBS)

project.o: project.c gfx.h
	$(CC) $(CFLAGS) -c project.c

clean:
	rm -f project project.o