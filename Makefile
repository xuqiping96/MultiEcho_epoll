OBJECTS = EchoServer EchoClient

all: $(OBJECTS)

.PHONY: all

EchoServer: EchoServer.c
	gcc -o $@ $< -lpthread

EchoClient: EchoClient.c
	gcc -o $@ $< -lpthread

clean:
	rm -f $(OBJECTS)