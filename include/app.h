#ifndef APP_H
#define APP_H

#include "./http.h"
#include "./hashmap.h"

#define INITIAL_ROUTE_CAPACITY 5

typedef http_response_t* (*route_handler)(http_request_t*);

typedef struct {
    char* method;
    char* path;
    route_handler handler;
    // hashmap* params;
} route_t;

typedef struct {
    route_t* routes;
    size_t route_count;
    size_t route_capacity;
} app_t;

app_t* app_create();
void app_run(app_t* app, int PORT);
void app_get(app_t* app, char* path, route_handler handler);
void app_post(app_t* app, char* path, route_handler handler);
void app_free(app_t* app);

#endif