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

void app_free(app_t* app) {
    route_t* routes = app->routes;
    
    for(size_t i = 0; i < app->route_count; i++) {
        route_free(&routes[i]);
    }

    free(routes);
    free(app->middleware);
}

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
        response = http_response_create(NOT_FOUND, NOT_FOUND_TEXT, TEXT_PLAIN, "404 Not Found");
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

bool double_route_capacity(app_t* app) {
    size_t new_capacity = app->route_capacity * 2;

    route_t* temp = realloc(app->routes, new_capacity);
    
    if(temp == NULL) return false;

    app->routes = temp;
    app->route_capacity = new_capacity;

    return true;
}

void app_add_route(app_t* app, char* method, char* path, route_handler handler) {
    if(app->route_count >= app->route_capacity) {
        bool has_doubled = double_route_capacity(app);
    
        if(!has_doubled) {
            perror("Failed to reallocate memory for routes, try again.");
            return;
        }
    }
    
    if(route_find_exact(app->routes, app->route_count, path, method) != NULL) {
        printf("Route already exists");
        return;
    }

    route_t route;
    if(!route_init(&route, method, path, handler)) {
        perror("Failed to create route");
        return;
    }

    printf("Registered GET %s\n", path);
    fflush(stdout);
    
    app->routes[app->route_count] = route;
    app->route_count++;
}

void app_post(app_t* app, char* path, route_handler handler) {
    app_add_route(app, "POST", path, handler);
}

void app_get(app_t* app, char* path, route_handler handler) {
    app_add_route(app, "GET", path, handler);
}

app_t* app_create() {
    app_t* app = malloc(sizeof(app_t));

    app->route_count = 0;
    app->route_capacity = INITIAL_ROUTE_CAPACITY;
    app->routes = malloc(INITIAL_ROUTE_CAPACITY * sizeof(route_t));
    
    if(app->routes == NULL) {
        perror("Failed to allocate memory for routes");
        return NULL;
    }

    app->middleware_count = 0;
    app->middleware_capacity = INITIAL_MIDDLEWARE_CAPACITY;
    app->middleware = malloc(INITIAL_MIDDLEWARE_CAPACITY * sizeof(middleware_handler));

    if(app->middleware == NULL) {
        perror("Failed to allocate memory for middleware");
        free(app->routes);
        return NULL;
    }

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

bool double_middleware_capacity(app_t* app) {
    size_t new_capacity = app->middleware_capacity * 2;
    middleware_handler* temp = realloc(
        app->middleware,
        new_capacity * sizeof(middleware_handler)
    );

    if(temp == NULL) {
        return false;
    }

    app->middleware = temp;
    app->middleware_capacity = new_capacity;

    return true;
}

void app_use(app_t* app, middleware_handler middleware) {
    if(app->middleware_count >= app->middleware_capacity) {
        if(!double_middleware_capacity(app)) {
            perror("Failed to reallocate memory for middleware");
            return;
        }
    }

    app->middleware[app->middleware_count++] = middleware;
}
