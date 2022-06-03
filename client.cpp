/*
Most of what I learned in this file (and have down) comes straight from:
    - "Beej's Guide to Network Programming"
    - https://beej.us/guide/bgnet/html/
*/

#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
using namespace std;

#define PORT "9191"     // port client will be connecting to

#define MAXDATASIZE 100     // max number of bytes we can send

// get sockaddr, IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in *)sa)->sin_addr);
    
    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[])
{
    int sockfd, numbytes;
    char buf[MAXDATASIZE];
    struct addrinfo hints, *servinfo, *p;
    char s[INET6_ADDRSTRLEN];

    if (argc != 2)
    {
        cout << "Usage: ./client.out <hostname>" << endl;
        exit(1);
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(argv[1], PORT, &hints, &servinfo) != 0)
    {
        fprintf(stderr, "getaddrinfo\n");
        exit(1);
    }

    // loop through results and connect to first one we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        {
            perror("socket");
            continue;
        }

        if  (connect(sockfd, p->ai_addr, p->ai_addrlen) < 0)
        {
            perror("connect");
            continue;
        }

        break;
    }

    if (p == NULL)
    {
        fprintf(stderr, "client: failed to connect\n");
        exit(2);
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
    cout << "client: connecting to " << s << endl;

    freeaddrinfo(servinfo); // all done with this now

    // receive data from server
    if((numbytes = recv(sockfd, buf, MAXDATASIZE - 1, 0)) < 0)
    {
        perror("recv");
        exit(EXIT_FAILURE);
    }

    buf[MAXDATASIZE] = '\0';

    // output data
    cout << "Client received: " << buf << endl;

    close(sockfd);

    return 0;
}