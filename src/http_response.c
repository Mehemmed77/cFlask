#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include "../include/http.h"
#include "../include/constants.h"

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
    char* value_copy = strdup(value ? value : "");

    if(value_copy == NULL) return false;

    header = get_response_header(response, name);

    if(header != NULL) {
        free(header->value);
        header->value = value_copy;
        return true;
    }

    header = malloc(sizeof(http_header_t));
    if(header == NULL) {
        free(value_copy);
        return false;
    }

    header->name = strdup(name ? name : "");
    if(header->name == NULL) {
        free(value_copy);
        free(header);
        return false;
    }

    header->value = value_copy;

    response->headers[response->header_count++] = header;

    return true;
}

bool double_response_buffer(char* response_buffer, size_t* capacity, size_t optional_capacity) {
    size_t new_capacity = optional_capacity == 0 ? *capacity * 2 : optional_capacity;
    char* temp = realloc(response_buffer, sizeof(char) * new_capacity);

    if(temp == NULL) {
        return false;
    }

    *capacity = new_capacity;
    response_buffer = temp;

    return true;
}

char* http_response_serialize(const http_response_t* response) {
    size_t capacity = 1024;
    char* response_buffer = malloc(sizeof(char) * capacity);

    if(response_buffer == NULL) return NULL;

    size_t total_len = 0;
    size_t len = 0;
    const char* status_text = response->status_text ? response->status_text : "";
    const char* body = response->body ? response->body : "";

    len = snprintf(NULL, 0, "HTTP/1.1 %u %s\r\n", response->status_code, status_text);

    snprintf(response_buffer, len + 1, "HTTP/1.1 %u %s\r\n", response->status_code, status_text);

    total_len += len;

    for(size_t i = 0; i < response->header_count; i++) {
        http_header_t* header = response->headers[i];
        const char* name = header->name ? header->name : "";
        const char* value = header->value ? header->value : "";
        
        len = snprintf(NULL, 0, "%s: %s\r\n", name, value);

        if(total_len + len + 1 >= capacity) {
            if(!double_response_buffer(response_buffer, &capacity, 0)) free(response_buffer);
        }

        snprintf(response_buffer + total_len, len + 1, "%s: %s\r\n", name, value);

        total_len += len;
    }

    len = snprintf(NULL, 0, "\r\n%s", body);

    if(total_len + len + 1 >= capacity) {
        if(!double_response_buffer(response_buffer, &capacity, total_len + len + 1)) free(response_buffer);
    }

    snprintf(response_buffer + total_len, len + 1, "\r\n%s", body);

    return response_buffer;
}

http_response_t* http_not_found_response(const char* body) {
    http_response_t* response = malloc(sizeof(http_response_t));

    if(response == NULL) return NULL;

    response->status_code = NOT_FOUND;
    response->status_text = strdup(NOT_FOUND_TEXT);
    response->body = strdup(body ? body : "");

    if(response->status_text == NULL || response->body == NULL) goto cleanup;

    response->headers = malloc(sizeof(http_header_t*) * INITIAL_HEADER_CAPACITY);

    response->header_capacity = INITIAL_HEADER_CAPACITY;
    response->header_count = 0;

    if(response->headers == NULL) goto cleanup;

    char content_length[32];
    snprintf(content_length, sizeof(content_length), "%zu", strlen(response->body));

    if(!set_header(response, "Content-Type", TEXT_PLAIN)) goto cleanup;
    if(!set_header(response, "Content-Length", content_length)) goto cleanup;

    return response;

    cleanup:
        http_response_free(response);
        return NULL;
}

http_response_t* http_text_response(const char* body) {
    http_response_t* response = malloc(sizeof(http_response_t));

    if(response == NULL) return NULL;

    response->status_code = OK;
    response->status_text = strdup(OK_TEXT);
    response->body = strdup(body ? body : "");

    if(response->status_text == NULL || response->body == NULL) goto cleanup;

    response->headers = malloc(sizeof(http_header_t*) * INITIAL_HEADER_CAPACITY);

    response->header_capacity = INITIAL_HEADER_CAPACITY;
    response->header_count = 0;

    if(response->headers == NULL) goto cleanup;

    char content_length[32];
    snprintf(content_length, sizeof(content_length), "%zu", strlen(response->body));

    if(!set_header(response, "Content-Type", TEXT_PLAIN)) goto cleanup;
    if(!set_header(response, "Content-Length", content_length)) goto cleanup;

    return response;

    cleanup:
        http_response_free(response);
        return NULL;
}

http_response_t* http_json_response(const char* json) {
    http_response_t* response = malloc(sizeof(http_response_t));

    if(response == NULL) return NULL;

    response->status_code = OK;
    response->status_text = strdup(OK_TEXT);
    response->body = strdup(json ? json : "");

    if(response->status_text == NULL || response->body == NULL) goto cleanup;

    response->headers = malloc(sizeof(http_header_t*) * INITIAL_HEADER_CAPACITY);

    response->header_capacity = INITIAL_HEADER_CAPACITY;
    response->header_count = 0;

    if(response->headers == NULL) goto cleanup;

    char content_length[32];
    snprintf(content_length, sizeof(content_length), "%zu", strlen(response->body));

    if(!set_header(response, "Content-Type", APPLICATION_JSON)) goto cleanup;
    if(!set_header(response, "Content-Length", content_length)) goto cleanup;

    return response;

    cleanup:
        http_response_free(response);
        return NULL;
}

void http_response_free(http_response_t* response) {
    if (!response) return;

    headers_free(response->headers, response->header_count);
    free(response->status_text);
    free(response->body);
    free(response);
}
