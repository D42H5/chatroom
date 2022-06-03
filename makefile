all: server.out client.out

server.out: server.cpp
	g++ server.cpp -Wall -o server.out

client.out: client.cpp
	g++ client.cpp -Wall -o client.out

clean:
	rm *.out