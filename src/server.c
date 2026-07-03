#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "../include/server.h"
#include "../include/connection.h"
#include "../include/http.h"
#include "../include/app.h"
#include "../include/hashmap.h"

http_response_t* home_handler(http_request_t* request) {
    char* category = hashmap_get(request->query_params, "category");

    if(category != NULL) {
        printf("%s\n", category);
    }

    return http_response_create(200, "OK", "text/html", "<!DOCTYPE html><html><head><title>Page Title</title></head><body><h1>This is a Heading</h1><p>This is a paragraph.</p></body></html>");
}

void print_route_segments(app_t* app) {
    for(size_t i = 0; i < app->route_count; i++) {
        route_t route = app->routes[i];

        printf("%s %s: %zu segment(s)\n", route.method, route.path, route.segment_count);

        for(size_t j = 0; j < route.segment_count; j++) {
            route_segment_t segment = route.segments[j];
            char* type = segment.type == ROUTE_SEG_PARAM ? "param" : "static";

            printf("  %s: %s\n", type, segment.value);
        }
    }
}

int main() {
    app_t* app = app_create();

    app_get(app, "/", home_handler);
    app_get(app, "/users", home_handler);
    app_get(app, "/users/:id", home_handler);
    app_get(app, "/users/:id/posts/:post_id", home_handler);

    print_route_segments(app);

    app_run(app, 8081);

    return 0;
}
