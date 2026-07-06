#include <stdio.h>
#include <stdlib.h>
#include "../include/app.h"

bool double_route_capacity(app_t* app) {
    size_t new_capacity = app->route_capacity * 2;

    route_t* temp = realloc(app->routes, new_capacity);
    
    if(temp == NULL) return false;

    app->routes = temp;
    app->route_capacity = new_capacity;

    return true;
}

void app_add_route(app_t* app, char* method, char* path, route_handler handler) {
    if(app->route_count >= app->route_capacity) {
        bool has_doubled = double_route_capacity(app);
    
        if(!has_doubled) {
            perror("Failed to reallocate memory for routes, try again.");
            return;
        }
    }
    
    if(route_find_exact(app->routes, app->route_count, path, method) != NULL) {
        printf("Route already exists");
        return;
    }

    route_t route;
    if(!route_init(&route, method, path, handler)) {
        perror("Failed to create route");
        return;
    }

    printf("Registered GET %s\n", path);
    fflush(stdout);
    
    app->routes[app->route_count] = route;
    app->route_count++;
}

void app_post(app_t* app, char* path, route_handler handler) {
    app_add_route(app, "POST", path, handler);
}

void app_get(app_t* app, char* path, route_handler handler) {
    app_add_route(app, "GET", path, handler);
}
