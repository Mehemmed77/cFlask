#ifndef ROUTE_H
#define ROUTE_H

#include <stdbool.h>
#include <stddef.h>
#include "http.h"

#define INITIAL_SEGMENT_CAPACITY 5

typedef http_response_t* (*route_handler)(http_request_t*);

typedef enum {
    ROUTE_SEG_STATIC,
    ROUTE_SEG_PARAM
} route_segment_type_t;

typedef struct {
    route_segment_type_t type;
    char* value;
} route_segment_t;

typedef struct {
    char* method;
    char* path;
    route_handler handler;

    route_segment_t* segments;
    size_t segment_count;
} route_t;

bool route_init(route_t* route, char* method, char* path, route_handler handler);
void route_free(route_t* route);

route_t* route_find(
    route_t* routes,
    size_t route_count,
    char* path,
    char* method,
    hashmap** params_out
);

route_t* route_find_exact(
    route_t* routes,
    size_t route_count,
    char* path,
    char* method
);

#endif
