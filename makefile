##
## @file makefile
##
## @brief this is the makefile for coffeemaker
##
## @author Ulrike Schaefer 1327450
##
## @date 01.04.2017
##
##

CC = gcc 
DEFS = -D_BSD_SOURCE -D_SVID_SOURCE -D_POSIX_C_SOURCE=200809
CFLAGS = -Wall -g -lrt -lpthread -std=c99 -pedantic $(DEFS)

.PHONY: all clean

all: server client

server: server.o coffeemaker.h

client: client.o coffeemaker.h

$.o: $.c 
	$( CC ) $( CFLAGS ) -c -o $@ $<

clean:
	rm -f server server.o client client.o

debug: CFLAGS += -DENDEBUG
debug: all



