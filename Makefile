CFLAGS = -Wall -Wextra -g -Wno-error=unused-variable

all: server subscriber

server: server.c
	gcc  $(CFLAGS) server.c -o server -lm

subscriber: subscriber.c
	gcc  $(CFLAGS) subscriber.c -o subscriber -lm

clean:
	rm server subscriber
