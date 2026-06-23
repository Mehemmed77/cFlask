#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../include/http.h"

#define INITIAL_HEADERS 5

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

