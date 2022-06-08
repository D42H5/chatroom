/*
Most of what I learned in this file (and have down) comes straight from:
    - "Beej's Guide to Network Programming"
    - https://beej.us/guide/bgnet/html/

Could not believe the quality of his content after I reached the dark depths of stack overflow
attempting to learn web sockets.

For the public ip address code, I got it from this stack overflow page:
https://stackoverflow.com/questions/44610978/popen-writes-output-of-command-executed-to-cout
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
#include <signal.h>
#include <sys/uio.h>
#include <string>
#include <array>
#include <vector>
#include <string>
using namespace std;

#define PORT "20510"     // port server will be hosted on

#define BACKLOG 10      // number of pending connections queue will allow

#define MAXDATASIZE 100   // max size of data to send/receive

void sigchld_handler(int s)
{
    // waitpid() might overwrite errno, so we save and restore it:
    int saved_errno = errno;

    while(waitpid(-1, NULL, WNOHANG) > 0);

    errno = saved_errno;
}

// get sockaddr , IPv4 or IPv6
void *get_in_addr(struct sockaddr *sa)
{
    if (sa->sa_family == AF_INET)
        return &(((struct sockaddr_in*)sa)->sin_addr);
    
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// function to print out public ip address for clients to use
void getIP(void);

int sendall(int s, string &buf)
{
    // int total {0};      // num bytes sent
    // int bytesleft {buf.size()};   // num bytes left to send
    int n;

    vector<char> v { buf.c_str(), buf.c_str() + buf.size() };
    uint32_t len = v.size();
    struct iovec iv[2] = { { &len, sizeof(len) }, { &v[0], len } };
    if( (n = writev(s, iv, 2)) < 0)
    {
        perror("writev");
    }
    // *len = total;      // return num bytes sent (can be used for checking in main)

    return (n == -1) ? -1 : 0;  // return -1 on failure, 0 on success
}

int recvall(int s, string &buf)
{
    uint32_t len;
    int n;

    recv(s, &len, sizeof(len), 0);

    vector<char> v (len + 1);
    n = recv(s, &v[0], len, 0);
    v[len + 1] = '\0';

    buf = &v[0];

    return (n == -1) ? -1 : 0;
}


int main()
{
    int sockfd, new_fd; // listen on sockfd, new connecdtion on new_fd
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_storage their_addr; // connector's address
    socklen_t sin_size;
    struct sigaction sa;
    int yes = 1;
    char s[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;     // Let computer tell which family to use
    hints.ai_socktype = SOCK_STREAM;    // Use sock stream
    hints.ai_flags = AI_PASSIVE;     // use my IP

    if(getaddrinfo(NULL, PORT, &hints, &servinfo) != 0)
    {
        perror("getaddrinfo");
        exit(EXIT_FAILURE);
    }

    // Loop through results and bind to first one we can
    for (p = servinfo; p != NULL; p = p->ai_next)
    {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) < 0)
        {
            perror("socket");
            continue;
        }

        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) < 0)
        {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }
        
        if (bind(sockfd, p->ai_addr, p->ai_addrlen) < 0)
        {
            perror("bind");
            continue;
        }

        break;
    }

    freeaddrinfo(servinfo);

    // If nothing binds, exit
    if (p == NULL)
    {
        fprintf(stderr, "Failed to bind\n");
        exit(EXIT_FAILURE);
    }

    if (listen(sockfd, BACKLOG) < 0)
    {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    sa.sa_handler = sigchld_handler; // reap all dead processes...
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) < 0) {
        perror("sigaction");
        exit(1);
    }

    // char hostname[20];
    // gethostname(hostname, sizeof hostname);
    // cout << "Hostname needed for client: " << hostname << endl;
    getIP();
    cout << "Server: waiting for connections..." << endl;

    while(1) // accept() loop!
    {
        string inbuf;
        string outbuf;
        sin_size = sizeof their_addr;
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd < 0)
        {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        // print who connection is to
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
        cout << "Server: got connection from " << s << endl;

        if(!fork()) // make child process
        {
            close(sockfd);  // child doesn't need listener socket

            int count {0};
            string temp {"Welcome to the chatroom! Enter the word 'Bye' to stop chatting!\n"};
            if (sendall(new_fd, temp) < 0)
            {
                perror("sendall");
                exit(0);
            }
            while(1)
            {
                // Send message to client
                if (count == 0)
                    { count++; }
                else
                {
                    cout << "To client: ";
                    getline(cin, outbuf);
                    if (sendall(new_fd, outbuf) < 0)
                    {
                        perror("send");
                        exit(0);
                    }
                }
            

                // Receive message from client
                if (recvall(new_fd, inbuf) < 0)
                {
                    perror("recv");
                    exit(0);
                }

                cout << "Client: " << inbuf << endl;
                if (inbuf == "Bye")
                {
                    cout << "Client has ended this conversation" << endl;
                    break;
                }
            }

            // Close new_fd and exit child process
            close(new_fd);
            exit(0);
        }
        close(new_fd);
    }

    close(sockfd);

    return 0;
}


void getIP(void)
{
    std::string command("curl whatismyip.akamai.com 2>&1");

    std::array<char, 128> buffer;
    std::string result;

    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe)
    {
        std::cerr << "Couldn't start ip-lookup command." << std::endl;
        exit(0);
    }
    int count {0};
    while (fgets(buffer.data(), 128, pipe) != NULL) {
        if (++count == 5)
            result += buffer.data();
    }
    pclose(pipe);

    std::cout << "Public IP for clients to use: " << result << std::endl;
}