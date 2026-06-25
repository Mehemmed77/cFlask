#ifndef CONNECTION_H
#define CONNECTION_H

#include <stddef.h>

typedef struct {
    char* buffer;
    size_t length;
} byte_buffer_t;

byte_buffer_t* connection_receive_request(int client_fd);
void connection_free(byte_buffer_t* byte_buffer);

#endif