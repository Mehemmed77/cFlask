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
#include "../include/constants.h"
#include "../include/hashmap.h"

bool run_middlewares(app_t* app, http_request_t* request, http_response_t** response) {
    for(size_t i = 0; i < app->middleware_count; i++) {
        if(!(app->middleware[i](request, response))) return false;
    }

    return true;
}

void handle_client(app_t* app, int client_fd) {
    char* raw_response = NULL;
    http_request_t* http_request = NULL;

    byte_buffer_t* byte_buffer = NULL;
    http_response_t* response = NULL;
    size_t response_length = 0;
    size_t total_sent = 0;

    byte_buffer = connection_receive_request(client_fd);
    if (byte_buffer == NULL) {
        perror("Failed to receive HTTP request");
        goto cleanup;
    }

    http_request = http_request_create(byte_buffer->buffer, byte_buffer->length);

    if (http_request == NULL || http_request->http_request_line == NULL) {
        perror("Failed to parse HTTP request");
        goto cleanup;
    }

    http_request_line_t* http_request_line = http_request->http_request_line;

    route_t* route = route_find(
        app->routes,
        app->route_count,
        http_request_line->path,
        http_request_line->method,
        &http_request->params
    );

    if(route == NULL) {
        response = http_not_found_response("404 Not Found");
        goto send_response;
    }

    if(!run_middlewares(app, http_request, &response)) goto send_response;

    response = route->handler(http_request);

    send_response:
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
