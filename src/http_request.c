#include <stdlib.h>
#include "../include/http.h"

void http_request_free(http_request_t* http_request) {
    if(http_request == NULL) return;

    free(http_request->body);
    http_request_headers_free(http_request->headers, http_request->header_count);
    http_request_line_free(http_request->http_request_line);
    free(http_request);
}

void http_request_line_free(http_request_line_t* http_request_line) {
    if (http_request_line == NULL) return;

    free(http_request_line->method);
    free(http_request_line->path);
    free(http_request_line->version);
    free(http_request_line->query_string);
    free(http_request_line);
}

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
