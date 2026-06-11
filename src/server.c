#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "../include/server.h"
#include "../include/connection.h"
#include "../include/http.h"

void handle_client(int client_fd) {
    byte_buffer_t* byte_buffer = NULL;
    http_response_t* response = NULL;
    char* raw_response = NULL;
    size_t response_length;
    size_t total_sent;

    byte_buffer = connection_receive_request(client_fd);
    if (byte_buffer == NULL) {
        perror("Failed to receive HTTP request");
        goto cleanup;
    }

    printf("%s\n", byte_buffer->buffer);

    response = http_response_create(200, "OK", "text/plain", "SALAM\n");
    if (response == NULL) {
        perror("Failed to create HTTP response");
        goto cleanup;
    }

    raw_response = http_response_serialize(response);
    if (raw_response == NULL) {
        perror("Failed to serialize HTTP response");
        goto cleanup;
    }

    response_length = strlen(raw_response);
    total_sent = 0;

    while (total_sent < response_length) {
        ssize_t bytes_sent = send(client_fd,
                                  raw_response + total_sent,
                                  response_length - total_sent,
                                  0);
        if (bytes_sent < 0) {
            perror("Failed to send HTTP response");
            goto cleanup;
        }

        total_sent += (size_t) bytes_sent;
    }

cleanup:
    if (client_fd >= 0) {
        close(client_fd);
    }
    connection_free(byte_buffer);
    http_free(response);
    free(raw_response);
}

void server_run(int PORT) {
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

        if(client_fd < 0) {
            perror("Accept failed");
            continue;
        }

        handle_client(client_fd);
    }
}

int main() {
    server_run(8080);

    return 0;
}
