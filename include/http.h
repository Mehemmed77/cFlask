#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include "hashmap.h"

#define INITIAL_HEADER_CAPACITY 5

typedef struct {
    const char* start;
    size_t len;
} http_header_bounds_t;

typedef struct {
    const char *request_line_start;
    size_t request_line_len;

    http_header_bounds_t* headers;
    size_t header_count;

    const char *body_start;
    size_t body_len;
} http_request_bounds_t;

typedef struct {
    char* method;
    char* path;
    char* version;
    char* query_string;
} http_request_line_t;

typedef struct {
    char* name;
    char* value;
} http_header_t;

typedef struct {
    unsigned int status_code;
    char* status_text;
    char* body;

    http_header_t** headers;
    size_t header_count;
    size_t header_capacity;

} http_response_t;

typedef struct {
    http_request_line_t* http_request_line;
    http_header_t** headers;
    size_t header_count;
    char *body;
    hashmap* query_params;
    hashmap* params;
} http_request_t;

/* Response */

char* http_response_serialize(const http_response_t* response);

void http_response_free(http_response_t* response);

http_response_t* http_text_response(const char* body);
http_response_t* http_json_response(const char* json);
http_response_t* http_not_found_response(const char* body);
// http_response_t* http_html_response(const char* html);

/* Request */

http_request_t* http_request_create(
    const char* raw_request,
    size_t raw_request_len
);

void trim_inplace(char* s);

void http_request_free(http_request_t* request);

void http_request_line_free(
    http_request_line_t* http_request_line
);

/* Parsing */

http_request_bounds_t* http_parse_boundaries(
    const char* buf,
    size_t raw_request_len
);

http_request_line_t* parse_request_line(
    char* raw_request_line
);

http_header_t* parse_http_header(
    char* line
);

char* parse_http_request_body(
    const char* raw_request,
    size_t body_length
);

/* Utilities */

size_t check_for_body_length(
    http_header_t** headers,
    size_t size
);

void headers_free(
    http_header_t** headers,
    size_t size
);

char* get_http_header_value(http_request_t* request, char* name);

void free_http_bounds(http_request_bounds_t* req);

#endif
