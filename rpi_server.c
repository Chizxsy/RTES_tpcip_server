#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <syslog.h>

int main(int argc, int argv[]){
    int listen_sockfd, client_sockfd;
    struct addrinfo hints, *res, *p;
    struct sockaddr_storage client_addr;
    socklen_t addr_size;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((getaddrinfo(NULL, PORT, &hints, &res)) != 0){
        syslog(LOG_ERR, "getaddrinfo");
        return -1;
    }
}