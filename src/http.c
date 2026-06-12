#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../include/http.h"

// UTILITIES START
char* http_response_serialize(const http_response_t* response) {
    size_t body_length = response->body ? strlen(response->body) : 0;

    size_t header_length = snprintf(NULL, 0,
        "HTTP/1.1 %u %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n%s",
        response->status_code,
        response->status_text ? response->status_text : "",
        response->content_type ? response->content_type : "",
        body_length,
        response->body ? response->body : ""
    );

    char* response_buffer = malloc(header_length + 1);
    if (!response_buffer) {
        return NULL;
    }

    snprintf(response_buffer, header_length + 1,
        "HTTP/1.1 %u %s\r\nContent-Type: %s\r\nContent-Length: %zu\r\n\r\n%s",
        response->status_code,
        response->status_text ? response->status_text : "",
        response->content_type ? response->content_type : "",
        body_length,
        response->body ? response->body : ""
    );

    return response_buffer;
}
// UTILITIES END

// this is just first version, where whole request consists of the request line
http_request_line_t* http_request_create(const char* raw_request) {
    if(raw_request == NULL) return NULL;

    http_request_line_t* http_request_line = malloc(sizeof(http_request_line_t));

    if(http_request_line == NULL) {
        perror("Memory allocation failed for http_request_line");
        return NULL;
    }

    http_request_line->method = NULL;
    http_request_line->path = NULL;
    http_request_line->version = NULL;


    short int index = 0;
    const char *delimiters = " \t\r\n";

    char* source = strdup(raw_request);
    char* token = strtok(source, delimiters);
    
    while(token != NULL && index != 3) {
        if (index == 0) http_request_line->method = strdup(token);
        
        else if(index == 1) http_request_line->path = strdup(token);
        
        else http_request_line->version = strdup(token);
        
        index++;
        token = strtok(NULL, delimiters);
    }

    free(source);

    if(!http_request_line->method || !http_request_line->path || !http_request_line->version) {
        perror("Failed to copy strings into http_request_line");
        http_request_line_free(http_request_line);
        return NULL;
    }

    printf("%s\n%s\n%s\n", http_request_line->method, http_request_line->path, http_request_line->version);

    return http_request_line;
}

http_response_t* http_response_create(
    int status,
    const char *text,
    const char *type,
    const char *body
) {
    http_response_t* response = malloc(sizeof(http_response_t));
    if (!response) {
        return NULL;
    }

    response->status_code = status;
    response->status_text = text ? strdup(text) : NULL;
    response->content_type = type ? strdup(type) : NULL;
    response->body = body ? strdup(body) : NULL;

    if ((text && !response->status_text) ||
        (type && !response->content_type) ||
        (body && !response->body)) {
        http_response_free(response);
        return NULL;
    }

    return response;
}

void http_response_free(http_response_t* response) {
    if (!response) return;

    free(response->status_text);
    free(response->content_type);
    free(response->body);
    free(response);
}

void http_request_line_free(http_request_line_t* http_request_line) {
    if (http_request_line == NULL) return;

    free(http_request_line->method);
    free(http_request_line->path);
    free(http_request_line->version);
    free(http_request_line);
}