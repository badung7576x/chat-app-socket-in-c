CFLAGS = -c -Wall
CC = gcc
LIBS =  -lm 
PTHREAD =  -g3 -pthread
GTK_LIB = -lgthread-2.0 `pkg-config gtk+-3.0 --cflags --libs`
OCLIENT     := client
OSERVER     := server

default: main

main:
	clear 
	${CC} -w -g -o ${OCLIENT} client.c event.c gui.c main.c ${GTK_LIB}
	${CC} -w -g -o ${OSERVER} server.c ${PTHREAD}
	
clean:
	rm -f client server