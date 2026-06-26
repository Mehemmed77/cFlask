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

http_response_t* home_handler(http_request_t* request) {
    return http_response_create(200, "OK", "text/plain", "Agilli olun\nMiyau");
}

int main() {
    app_t* app = app_create();
    app_get(app, "/", home_handler);

    app_run(app, 8080);

    return 0;
}
