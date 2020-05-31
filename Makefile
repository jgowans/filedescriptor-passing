CC = gcc
CFLAGS = -g -Wall

.PHONY: all
all: init sender receiver

init: init.c shared-header.h
	$(CC) $(CFLAGS) -o $@ $<

sender: sender.c shared-header.h
	$(CC) $(CFLAGS) -o $@ $<

receiver: receiver.c shared-header.h
	$(CC) $(CFLAGS) -o $@ $<
.PHONY: clean
clean:
	rm -f init sender receiver
