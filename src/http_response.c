#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "../include/http.h"

char* serialize_status_line(unsigned int status_code, char* status_text) {
    size_t len = snprintf(NULL, 0, "HTTP/1.1 %u %s\r\n", status_code, status_text);

    char* response_buffer = malloc(len + 1);
    if(!response_buffer) return NULL;

    snprintf(response_buffer, len + 1, "HTTP/1.1 %u %s\r\n", status_code, status_text);

    return response_buffer;
}

bool double_http_response_header_capacity(http_response_t* response) {
    size_t new_capacity = response->header_capacity * 2;

    http_header_t** temp = realloc(response->headers, sizeof(http_header_t*) * new_capacity);

    if(temp == NULL) return false;

    response->headers = temp;
    response->header_capacity = new_capacity;

    return true;
}

http_header_t* get_response_header(http_response_t* response, const char* name) {
    for(size_t i = 0; i < response->header_count; i++) {
        http_header_t* header = response->headers[i];

        if(strcmp(name, header->name) == 0) return header;
    }

    return NULL;
}

bool set_header(http_response_t* response, const char* name, const char* value) {
    if(response->header_capacity <= response->header_count) {
        if(!(double_http_response_header_capacity(response))) return false;
    }

    http_header_t* header;

    header = get_response_header(response, name);

    if(header != NULL) {
        header->value = value;
        return true;
    }

    header = malloc(sizeof(http_header_t));
    if(header == NULL) return false;

    header->name = name;
    header->value = value;

    response->headers[response->header_count++] = header;

    return true;
}

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
    const char* text,
    const char* type,
    const char *body
) {
    http_response_t* response = malloc(sizeof(http_response_t));
    if (!response) {
        return NULL;
    }

    response->status_code = status;
    response->status_text = text ? strdup(text) : NULL;
    response->body = body ? strdup(body) : NULL;

    response->headers = malloc(sizeof(http_header_t*));

    response->header_capacity = INITIAL_HEADER_CAPACITY;
    response->header_count = 0;

    if ((text && !response->status_text) ||
        (body && !response->body) || !response->headers) {
        http_response_free(response);
        return NULL;
    }

    return response;
}

void http_response_free(http_response_t* response) {
    if (!response) return;

    free(response->status_text);
    headers_free(response->headers, response->header_count);
    free(response->body);
    free(response);
}
