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
        printf("query category: %s\n", category);
    }

    return http_response_create(200, "OK", "text/plain", "home");
}

http_response_t* users_handler(http_request_t* request) {
    (void) request;

    return http_response_create(200, "OK", "text/plain", "users index");
}

http_response_t* user_detail_handler(http_request_t* request) {
    char* id = hashmap_get(request->params, "id");

    if(id != NULL) {
        printf("route param id: %s\n", id);
    }

    return http_response_create(200, "OK", "text/plain", id != NULL ? id : "missing id");
}

http_response_t* post_detail_handler(http_request_t* request) {
    char* id = hashmap_get(request->params, "id");
    char* post_id = hashmap_get(request->params, "post_id");

    if(id != NULL && post_id != NULL) {
        printf("route params id=%s post_id=%s\n", id, post_id);
    }

    return http_response_create(200, "OK", "text/plain", "post detail");
}

int main() {
    app_t* app = app_create();

    app_get(app, "/", home_handler);
    app_get(app, "/users", users_handler);
    app_get(app, "/users/:id", user_detail_handler);
    app_get(app, "/users/:id/posts/:post_id", post_detail_handler);

    app_run(app, 8081);

    return 0;
}
