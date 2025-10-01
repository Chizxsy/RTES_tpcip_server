#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <syslog.h>

#define PORT "54321"
#define BUFFER_SIZE 1024




int send_data(int sockfd, const char *filename){
    char buffer[BUFFER_SIZE];
    size_t bytes_read;

    FILE *fp = fopen(filename, "rb");
    if (fp == NULL){
        syslog(LOG_ERR, "Failed to open file");
        return -1;
    }

    while ((bytes_read = fread(buffer, 1, sizeof(buffer), fp))>0){
        if (send(sockfd, buffer, bytes_read, 0) == -1){
            syslog(LOG_ERR, "send");
            fclose(fp);
            return -1;
        }
    }
    fclose(fp);
    return 0;
}

int receive_data(int sockfd){
    char buffer[BUFFER_SIZE];
    int bytes_recv;
    // leave room for null terminator
    while ((bytes_recv = recv(sockfd, buffer, sizeof(buffer) - 1, 0)) > 0){
        // null terminate bytes recvieved to print
        buffer[bytes_recv] = '\0';
        syslog(LOG_INFO, "%s", buffer);
        }

        if (bytes_recv == -1){
            syslog(LOG_ERR, "recv");
            return -1;
        }
    
}

int main(int argc, char *argv[]) {
    int client_sockfd, status; 
    int sockarg;
    struct addrinfo hints, *res, *p;
    struct sockaddr_storage serveraddr;
    socklen_t serveraddr_len;
    struct linger opt;

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <hostname> <filepath>\n", argv[0]);
        return -1;
    }

    const char *hostname = argv[1];
    const char *filepath = argv[2];

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((status = getaddrinfo(hostname, PORT, &hints, &res)) != 0) {
        syslog(LOG_ERR, "getaddrinfo: %s", gai_strerror(status));
        return -1;
    }

    opt.l_onoff = 1;
    opt.l_linger = 0;
    sockarg = 1;

    for (p=res; p!=NULL; p=p->ai_next){
        if((client_sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) continue;

        setsockopt(client_sockfd, SOL_SOCKET, SO_REUSEADDR, (char*)&sockarg, sizeof(sockarg));
        setsockopt(client_sockfd, SOL_SOCKET, SO_LINGER, (char*)&opt, sizeof(opt));

        if (connect(client_sockfd, p->ai_addr, p->ai_addrlen) == -1){
            syslog(LOG_ERR, "Client connect");
            continue;
        }

        break;

    }
    freeaddrinfo(res);

    if (p == NULL){
        syslog(LOG_ERR, "Client connection failed");
        return -1;
    }

    syslog(LOG_INFO, "Connected to %s\n", hostname);

    if (send_data(client_sockfd, filepath) == 0) {
        printf("File sent successfully.\n");
    } else {
        fprintf(stderr, "Error sending file.\n");
    }

    shutdown(client_sockfd, SHUT_WR);

    printf("\n--- Waiting for server response ---\n");
    receive_data(client_sockfd);



    close(client_sockfd);

}
