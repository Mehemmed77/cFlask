#include <stdio.h>
#include <stdlib.h>
#include "../include/app.h"

bool double_middleware_capacity(app_t* app) {
    size_t new_capacity = app->middleware_capacity * 2;
    middleware_handler* temp = realloc(
        app->middleware,
        new_capacity * sizeof(middleware_handler)
    );

    if(temp == NULL) {
        return false;
    }

    app->middleware = temp;
    app->middleware_capacity = new_capacity;

    return true;
}

void app_use(app_t* app, middleware_handler middleware) {
    if(app->middleware_count >= app->middleware_capacity) {
        if(!double_middleware_capacity(app)) {
            perror("Failed to reallocate memory for middleware");
            return;
        }
    }

    app->middleware[app->middleware_count++] = middleware;
}
