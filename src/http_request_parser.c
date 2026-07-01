#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include "../include/http.h"
#include "../include/hashmap.h"

char* parse_http_request_body(const char* raw_request, size_t body_length) {
    char *body_copy = NULL;

    if(raw_request && body_length) {
        body_copy = malloc(body_length + 1);
        if(body_copy) {
            memcpy(body_copy, raw_request, body_length);
            body_copy[body_length] = '\0';
        }
    }

    return body_copy;
}

http_header_t* parse_http_header(char* line) {
    char* colon = strchr(line, ':');

    if(!colon) return NULL;

    *colon = '\0';

    char* name = strdup(line);
    char* value = strdup(colon + 1);

    trim_inplace(name);
    trim_inplace(value);

    http_header_t* header = malloc(sizeof(http_header_t));

    header->name = name;
    header->value = value;

    return header;
}

http_request_line_t* parse_request_line(char* raw_request_line) {
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

        else if(index == 1) {
            char* query_start = strchr(token, '?');
            char* query_string = NULL;

            if(query_start != NULL) {
                *query_start = '\0';

                query_string = strdup(query_start + 1);
            }

            http_request_line->path = strdup(token);
            http_request_line->query_string = query_string;
        }

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

hashmap* parse_query_params(char* query_string) {
    char* source;
    hashmap* map;

    const char* delimiters = "&";
    source = strdup(query_string);
    if(!source) goto cleanup;
    
    map = hashmap_create();
    
    if(!map) goto cleanup;

    char* token = strtok(source, delimiters);
    
    while(token != NULL) {
        char* equal = strchr(token, '=');

        if(equal == NULL) goto cleanup;

        *equal = '\0';
        
        hashmap_put(map, token, strdup(equal + 1));

        token = strtok(NULL, delimiters);
    }

    free(source);

    return map;

    cleanup:
        free(source);
        hashmap_destroy(map);
        return NULL;
}

http_request_t* http_request_create(const char* raw_request, size_t raw_request_len) {
    http_request_bounds_t* req = http_parse_boundaries(raw_request, raw_request_len);
    if(req == NULL) return NULL;

    hashmap* map = NULL;
    http_request_line_t* request_line = NULL;
    http_header_t** headers = NULL;
    size_t temp_size = 0;
    char* body = NULL;

    char* req_line_char = malloc(sizeof(char) * (req->request_line_len + 1));
    if(req_line_char == NULL) goto cleanup_bounds;

    memcpy(req_line_char, req->request_line_start, req->request_line_len);
    req_line_char[req->request_line_len] = '\0';

    request_line = parse_request_line(req_line_char);
    free(req_line_char);

    map = parse_query_params(request_line->query_string);
    if(map == NULL) goto cleanup_bounds;

    if(request_line == NULL) goto cleanup_bounds;

    headers = malloc(req->header_count * sizeof(*headers));

    if(headers == NULL) goto cleanup_bounds;

    for(size_t i = 0; i < req->header_count; i++) {
        char* header_line = malloc(sizeof(char) * (req->headers[i].len + 1));
        
        if(header_line == NULL) {
            goto cleanup_bounds;
        };
        
        memcpy(header_line, req->headers[i].start, req->headers[i].len);
        header_line[req->headers[i].len] = '\0';
        
        http_header_t* header = parse_http_header(header_line);
        free(header_line);
        headers[i] = header;

        temp_size++;
    }

    body = parse_http_request_body(req->body_start, req->body_len);

    http_request_t* request = malloc(sizeof(http_request_t));
    if (request == NULL) goto cleanup_bounds;

    request->body = body;
    request->http_request_line = request_line;
    request->headers = headers;
    request->header_count = req->header_count;
    request->query_params = map;

    fflush(stdout);

    free_http_bounds(req);

    return request;

    cleanup_bounds:
        free(body);
        hashmap_destroy(map);
        http_request_line_free(request_line);
        http_request_headers_free(headers, temp_size);
        free_http_bounds(req);
        return NULL;
}
