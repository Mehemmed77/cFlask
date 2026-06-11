#ifndef HTTP_H
#define HTTP_H

typedef struct {
    unsigned int status_code;
    char* status_text;
    char* content_type;
    char* body;
} http_response_t;

char* http_response_serialize(const http_response_t* response);
void http_free(http_response_t* response);

http_response_t* http_response_create(
    int status,
    const char *text,
    const char *type,
    const char *body
);

#endif