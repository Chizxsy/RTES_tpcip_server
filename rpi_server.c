#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <syslog.h>

#define PORT "54321"
#define BUFFER_SIZE 1024

int receive_data(int sockfd, const char *filename){
     char buffer[BUFFER_SIZE];
     int bytes_recv;

     FILE *fp = fopen(filename, "wb");
     if (fp == NULL){
        syslog(LOG_ERR, "fopen");
        return -1;
     }


}

int main(int argc, int argv[]){
    int listen_sockfd, client_sockfd;
    struct addrinfo hints, *res, *p;
    struct sockaddr_storage client_addr;
    socklen_t addr_size;

    signal(SIGCHLD, SIG_IGN);
    
    openlog("TCP server", LOG_PID, LOG_USER);

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((getaddrinfo(NULL, PORT, &hints, &res)) != 0){
        syslog(LOG_ERR, "getaddrinfo");
        return -1;
    }

    if ((listen_sockfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol)) == -1){
        syslog(LOG_ERR, "socket");
        return -1;
    }

    int sockarg = 1;
    setsockopt(client_sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&sockarg, sizeof(sockarg));

    if (bind(listen_sockfd, res->ai_addr, res->ai_addrlen) == -1){
        syslog(LOG_ERR, "bind");
        close(listen_sockfd);
        return -1;
    }

    freeaddrinfo(res);

    if (listen(listen_sockfd, 10) == -1){
        syslog(LOG_ERR, "listen");
        return -1;
    }

    while (1){
        addr_size = sizeof(client_addr);
        client_sockfd = accept(listen_sockfd, (struct sockaddr *)&client_addr, &addr_size);
        if (client_sockfd == -1){
            syslog(LOG_ERR, "accept");
            continue;
        }
        syslog(LOG_INFO, "Accepted connection");

        if (fork() == 0){
            close(listen_sockfd);

            char filename[128];
            sprintf(filename, "receivedgcode_%d.gcode", listen_sockfd);

            syslog(LOG_INFO, "File saved to %s", filename);

            if (receive_data(client_sockfd, filename) == 0){
                syslog(LOG_INFO, "File received succesfully");
                send(client_sockfd, "File received", 14, 0);
            } else {
                syslog(LOG_ERR, "Error receiving file");
            }

            close(client_sockfd);
        }
        close(client_sockfd);
    }
    closelog();
    return 0;
}
