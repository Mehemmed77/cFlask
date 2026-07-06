#include <stdio.h>
#include <stdlib.h>
#include "../include/app.h"

void app_free(app_t* app) {
    route_t* routes = app->routes;
    
    for(size_t i = 0; i < app->route_count; i++) {
        route_free(&routes[i]);
    }

    free(routes);
    free(app->middleware);
}

app_t* app_create() {
    app_t* app = malloc(sizeof(app_t));

    app->route_count = 0;
    app->route_capacity = INITIAL_ROUTE_CAPACITY;
    app->routes = malloc(INITIAL_ROUTE_CAPACITY * sizeof(route_t));
    
    if(app->routes == NULL) {
        perror("Failed to allocate memory for routes");
        return NULL;
    }

    app->middleware_count = 0;
    app->middleware_capacity = INITIAL_MIDDLEWARE_CAPACITY;
    app->middleware = malloc(INITIAL_MIDDLEWARE_CAPACITY * sizeof(middleware_handler));

    if(app->middleware == NULL) {
        perror("Failed to allocate memory for middleware");
        free(app->routes);
        return NULL;
    }

    return app;
}
