#ifndef HTTP_H
#define HTTP_H

typedef struct {
    unsigned int status_code;
    char* status_text;
    char* content_type;
    char* body;
} http_response_t;

typedef struct {
    char *method;
    char *path;
    char *version;
} http_request_line_t;

typedef struct {
    char* name;
    char* value;
} http_header_t;

typedef struct {
    http_request_line_t* http_request_line;
    http_header_t** headers;
    size_t header_count;
    char *body;
} http_request_t;

char* http_response_serialize(const http_response_t* response);
void http_response_free(http_response_t* response);
void http_request_line_free(http_request_line_t* http_request_line);

http_response_t* http_response_create(
    int status,
    const char *text,
    const char *type,
    const char *body
);

http_request_t* http_request_create(const char* raw_request);
void http_request_free(http_request_t* request);

#endif