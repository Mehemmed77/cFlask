#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "../include/http.h"
#include <stdbool.h>

#define INITIAL_HEADERS 5

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

http_request_line_t* parse_request_line (char* raw_request_line) {
    if(raw_request_line == NULL) return NULL;
    
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
    
    char* source = strdup(raw_request_line);
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

    return http_request_line;
}

static void trim_inplace(char* s) {
    if (!s) return;

    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);

    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len-1])) s[--len] = '\0';
}

http_header_t* parse_http_header(char* line) {
    char* colon = strchr(line, ':');

    printf("%s", line);

    if(!colon) return NULL;

    *colon = '\0';

    char* name = strdup(line);
    char* value = strdup(colon + 1);

    trim_inplace(name);
    trim_inplace(value);

    http_header_t* header = malloc(sizeof(http_header_t));

    printf("Name: %s, Value: %s\n", name, value);

    header->name = name;
    header->value = value;

    return header;
}

size_t check_for_body_length(http_header_t** headers, size_t size) {
    http_header_t* header = NULL;
    size_t result = 0;

    for(int i = 0; i < size; i++) {
        header = headers[i];

        // ignoring Transfer-Encoding: chunked header for now
        if(strcmp("Content-Length", header->name) == 0) {
            sscanf(header->value, "%zu", &result);
            return result;
        }
    }

    return 0;
}

char* parse_http_request_body(const char* raw_request, size_t body_length) {
    const char* raw_body = NULL;

    if(raw_request) {
        const char *sep = strstr(raw_request, "\r\n\r\n");
        raw_body = sep ? sep + 4 : NULL;
    }

    char *body_copy = NULL;

    if(raw_body && body_length) {
        body_copy = malloc(body_length + 1);
        if(body_copy) {
            memcpy(body_copy, raw_body, body_length);
            body_copy[body_length] = '\0';
        }
    } else if (raw_body) {
        body_copy = strdup(raw_body);
    }

    return body_copy;
}

http_request_bounds_t* http_parse_boundaries(const char* buf, size_t buf_len) {
    http_request_bounds_t* http_request_bounds = malloc(sizeof(http_request_bounds_t));

    if(http_request_bounds == NULL) {
        perror("Failed to allocate memory for http request bounds");
        goto cleanup;
    }

    size_t capacity = INITIAL_HEADERS;
    http_request_bounds->headers = malloc(INITIAL_HEADERS * sizeof(http_header_bounds_t));

    if(http_request_bounds->headers == NULL) {
        perror("Failed to allocate memory for http header bounds");
        goto cleanup;
    }

    const char* ptr = buf;
    const char* end = buf + buf_len;

    const char* request_line_start = buf;
    size_t request_line_len = 0;

    while(ptr + 1 < end && !(ptr[0] == '\r' && ptr[1] == '\n')) {
        request_line_len++;
        ptr++;
    }

    ptr += 2;

    http_request_bounds->request_line_start = request_line_start;
    http_request_bounds->request_line_len = request_line_len;

    size_t len = 0;
    size_t header_count = 0;
    
    while(
        ptr + 3 < end &&
        !(ptr[0] == '\r' && ptr[1] == '\n' && ptr[2] == '\r' && ptr[3] == '\n')
    ) {

        if (ptr + 1 >= end || (*ptr == '\r' && ptr[1] == '\n')) break;

        printf("%c\n", *ptr);
        if(header_count >= capacity) {
            capacity *= 2;
            http_header_bounds_t* temp = realloc(http_request_bounds->headers, capacity * sizeof(http_header_bounds_t));
            if (temp == NULL) {
                perror("Failed to reallocate memory for http header bounds");
                goto cleanup;
            }
            http_request_bounds->headers = temp;
        }

        http_request_bounds->headers[header_count].start = ptr;
        
        while(ptr + 1 < end && !(*ptr == '\r' && *(ptr + 1) == '\n')) {
            ptr++;
            len++;
        }
        
        http_request_bounds->headers[header_count].len = len;

        len = 0;
        ptr += 2;

        header_count++;
    }

    ptr += 4;
    
    http_request_bounds->header_count = header_count;
    http_request_bounds->body_start = ptr;
    http_request_bounds->body_len = strlen(ptr);

    return http_request_bounds;

cleanup:
    if (http_request_bounds) {
        free(http_request_bounds->headers);
        free(http_request_bounds);
    }
    return NULL;
}

// UTILITIES END

void http_request_headers_free(http_header_t** headers, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (headers[i]) {
            free(headers[i]->name);
            free(headers[i]->value);
            free(headers[i]);
        }
    }

    free(headers);
}

http_request_t* http_request_create(const char* raw_request, size_t raw_request_len) {
    http_request_bounds_t* req = http_parse_boundaries(raw_request, raw_request_len);

    printf("=== HTTP REQUEST BOUNDS ===\n");

    printf("request_line_len = %zu\n", req->request_line_len);
    printf("request_line     = '%.*s'\n",
           (int)req->request_line_len,
           req->request_line_start);

    printf("header_count     = %zu\n", req->header_count);

    for (size_t i = 0; i < req->header_count; i++) {
        printf("header[%zu] len=%zu value='%.*s'\n",
               i,
               req->headers[i].len,
               (int)req->headers[i].len,
               req->headers[i].start);
    }

    printf("body_len         = %zu\n", req->body_len);
    printf("body             = '%.*s'\n",
           (int)req->body_len,
           req->body_start ? req->body_start : "");

    printf("===========================\n");

    char* req_line = malloc(sizeof(char) * (req->request_line_len + 1));
    memcpy(req_line, req->request_line_start, req->request_line_len);

    http_request_line_t* request_line = parse_request_line(req_line);

    http_header_t** headers = malloc(req->header_count * sizeof(*headers));

    for(int i = 0; i < req->header_count; i++) {
        char* header_line = malloc(sizeof(char) * (req->headers[i].len + 1));

        memcpy(header_line, req->headers[i].start, req->headers[i].len);

        http_header_t* header = parse_http_header(header_line);
        headers[i] = header;
    }

    char* body = parse_http_request_body(req->body_start, req->body_len);

    http_request_t* request = malloc(sizeof(http_request_t));

    request->body = body;
    request->http_request_line = request_line;
    request->headers = headers;
    request->header_count = req->header_count;

    printf("Method: %s, Path: %s, Version: %s", request_line->method, request_line->path, request_line->version);
    fflush(stdout);

    return request;
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

// void http_request_bounds_free(http_request_bounds_t* http_request_bounds) {
//     if (!http_request_bounds) return;
    
//     free(http_request_bounds->headers);
//     free(http_request_bounds);
// }

void http_request_free(http_request_t* http_request) {
    if(http_request == NULL) return;

    free(http_request->body);
    http_request_headers_free(http_request->headers, http_request->header_count);
    http_request_line_free(http_request->http_request_line);
    free(http_request);
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