#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#define PORT 8080

void client() {
    int status, client_fd;
    client_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (client_fd < 0) {
        perror("Failed to create socket\n");
        exit(EXIT_FAILURE);
    }

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if ((status = connect(client_fd, (struct sockaddr*) &server_addr, sizeof(server_addr)) < 0)) {
        perror("Connection failed!\n");
        exit(EXIT_FAILURE);
    }

    char *request = "Hello from client";
    send(client_fd, request, strlen(request), 0);

    size_t BUFFER_SIZE = 1024;
    char *buffer = malloc(BUFFER_SIZE * sizeof(char));

    int bytes_received = recv(client_fd, buffer, BUFFER_SIZE, 0);

    if (bytes_received > 0) {
        buffer[bytes_received] = '\0';

        printf("%s\n", buffer);
    }

    else if(bytes_received == 0) {
        printf("Server closed connection.\n");
    }

    else {
        perror("Receive Failed\n");
    }

    printf("Connection Succeeded!\n");
}

int main() {
    client();

    return 0;
}
