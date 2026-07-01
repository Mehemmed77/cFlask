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
    printf("%s\n", (char*) hashmap_get(request->query_params, "category"));

    return http_response_create(200, "OK", "text/html", "<!DOCTYPE html><html><head><title>Page Title</title></head><body><h1>This is a Heading</h1><p>This is a paragraph.</p></body></html>");
}

int main() {
    app_t* app = app_create();
    app_get(app, "/", home_handler);

    app_run(app, 8081);

    return 0;
}
