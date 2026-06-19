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

http_request_line_t* parse_request_line(const char* raw_request_line) {
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

http_request_line_t* return_parsed_http_line(char* str) {

}

http_request_t* http_request_create(const char* raw_request) {
    http_request_t* http_request = NULL;
    http_header_t** headers = NULL;
    http_header_t* http_header = NULL; 

    char* source = strdup(raw_request);

    char* body_start = strstr(source, "\r\n\r\n");
    if (body_start) {
        *body_start = '\0';
    }

    http_request = malloc(sizeof(http_request_t));

    if(http_request == NULL) {
        perror("Failed to allocate memory for http_request");
        goto cleanup;
    }

    headers = malloc(5* sizeof(*headers));

    if(headers == NULL) {
        perror("Failed to allocate memory for headers");
        goto cleanup;
    }

    size_t capacity = INITIAL_HEADERS;

    http_request->http_request_line = parse_request_line(token);

    if (!http_request->http_request_line) {
        goto cleanup;
    }

    token = strtok(NULL, delimiter);

    size_t header_index = 0;

    while(token != NULL) {
        if(header_index == 4) break;

        if(strlen(token) == 0) {
            printf("%s", token);
            fflush(stdout);
            token = strtok(NULL, delimiter);
            continue;
        }

        http_header = parse_http_header(token);

        if (!http_header) {
            perror("Header couldn't be parsed");
            goto cleanup;
        }
        
        if ((size_t) header_index >= capacity) {
            capacity *= 2;
            http_header_t** temp = realloc(headers, capacity * sizeof(*headers));

            if (temp == NULL) {
                perror("Reallocation failed");
                free(http_header->name);
                free(http_header->value);
                free(http_header);
                goto cleanup;
            }
            
            headers = temp;
        }

        headers[header_index] = http_header;
        
        token = strtok(NULL, delimiter);
        header_index++;
    }

    http_request->headers = headers;
    http_request->header_count = header_index;

    size_t body_length = check_for_body_length(headers, http_request->header_count);

    http_request->body = parse_http_request_body(raw_request, body_length);

    free(source);
    return http_request;

    cleanup:
        if (http_request) {
            if (http_request->http_request_line) {
                http_request_line_free(http_request->http_request_line);
            }
            free(http_request->body);
            free(http_request);
        }
        if (headers) http_request_headers_free(headers, header_index);
        free(source);
        return NULL;
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