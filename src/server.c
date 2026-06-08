#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "../include/server.h"
#define PORT 8080

void server()
{
    size_t BUFFER_SIZE = 1024;
    struct sockaddr_in address;
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if(server_fd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    // int status = inet_pton(AF_INET, "127.0.0.1", &address.sin_addr);
    // if(status < 0) {
    //     if (status == 0) printf("Invalid IP address string format\n");
    //     else perror("inet_pton failed");
    // }

    if(bind(server_fd, (struct sockaddr*) &address, sizeof(address)) < 0) {
        perror("Bind failed , port might be already be in use!");
        exit(EXIT_FAILURE);
    }

    if(listen(server_fd, 10) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);   
    }

    printf("Listening on Port: %d\n", PORT);
    fflush(stdout);

    while(1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);

        char *buffer = malloc(BUFFER_SIZE * sizeof(char));
        
        int bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);
        buffer[bytes_received] = '\0';

        printf("%s\n", buffer);

        char *reply = "Aleykum Salam";
        send(client_fd, reply, strlen(reply), 0);
    }
}

int main() {
    server();

    return 0;
}
