#ifndef APP_H
#define APP_H

#include <stddef.h>
#include "./route.h"

#define INITIAL_ROUTE_CAPACITY 5
#define INITIAL_MIDDLEWARE_CAPACITY 3

typedef bool (*middleware_handler) (
    http_request_t* request,
    http_response_t** response
);

typedef struct {
    route_t* routes;
    size_t route_count;
    size_t route_capacity;

    size_t middleware_count;
    size_t middleware_capacity;
    middleware_handler* middleware;
} app_t;

app_t* app_create();
void app_run(app_t* app, int PORT);
void app_get(app_t* app, char* path, route_handler handler);
void app_post(app_t* app, char* path, route_handler handler);
void app_free(app_t* app);
void app_use(app_t* app, middleware_handler handler);

#endif
