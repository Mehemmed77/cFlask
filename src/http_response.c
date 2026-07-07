#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../include/http.h"

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
