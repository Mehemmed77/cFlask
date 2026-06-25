#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "../include/http.h"
#include "../include/app.h"
#include "../include/connection.h"

#define INITIAL_ROUTE_CAPACITY 5

// UTILITIES
bool route_exists(app_t* app, char* path, char* method) {
    route_t* routes = app->routes;

    for(size_t i = 0; i < app->route_count; i++) {
        if(
            !(strcmp(routes[i].method, method) || 
            strcmp(routes[i].path, path))) return true;
    }

    return false;
}

void handle_client(app_t* app, int client_fd) {
    char* raw_response = NULL;

    byte_buffer_t* byte_buffer = NULL;
    http_response_t* response = NULL;
    size_t response_length = 0;
    size_t total_sent = 0;

    byte_buffer = connection_receive_request(client_fd);
    if (byte_buffer == NULL) {
        perror("Failed to receive HTTP request");
        goto cleanup;
    }

    http_request_t* http_request = http_request_create(byte_buffer->buffer, byte_buffer->length);

    if (http_request == NULL || http_request->http_request_line == NULL) {
        perror("Failed to parse HTTP request");
        goto cleanup;
    }

    http_request_line_t* http_request_line = http_request->http_request_line;

    if(!route_exists(app, http_request_line->path, http_request_line->method)) {
        printf("==================== ROUTE DOES NOT EXIST ===================");
    };

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
        http_response_free(response);
        http_request_free(http_request);
        free(raw_response);
}

app_t* app_create() {
    app_t* app = malloc(sizeof(app_t));

    app->routes = 0;
    app->route_capacity = INITIAL_ROUTE_CAPACITY;
    app->routes = malloc(INITIAL_ROUTE_CAPACITY * sizeof(route_t));
    app->route_count = 0;

    return app;
}

void app_run(app_t* app, int PORT) {
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

        handle_client(app, client_fd);
    }
}
