.PHONY: all client server clean

all: client server

client:
	$(MAKE) -C client

server:
	$(MAKE) -C server

clean:
	$(MAKE) -C client clean
	$(MAKE) -C server clean

runclient: client
	./client/bin/client.exe

runserver: server
	./server/bin/server.exe
