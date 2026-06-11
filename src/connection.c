#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include "../include/connection.h"

bool should_stop(char* buffer, int total_bytes_received) {
    if (total_bytes_received < 4) return false;

    char *end_ptr = buffer + total_bytes_received - 4;

    return end_ptr[0] == '\r' && end_ptr[1] == '\n' && end_ptr[2] == '\r' && end_ptr[3] == '\n';
}

byte_buffer_t* connection_receive_request(int client_fd) {
    size_t BUFFER_SIZE = 1024;
    char* buffer = NULL;
    char* temp = NULL;
    int bytes_received;
    int total_bytes_received = 0;
    int index = 1;

    do{
        temp = realloc(buffer, (BUFFER_SIZE * index + 1) * sizeof(char));

        if (temp == NULL) {
            free(buffer);
            return NULL;
        };

        buffer = temp;

        bytes_received = recv(client_fd, buffer + total_bytes_received, BUFFER_SIZE, 0);

        // currently I'm avoiding complex error handling
        if (bytes_received <= 0) {
            free(buffer);
            return NULL;
        };

        total_bytes_received += bytes_received;

        buffer[total_bytes_received] = '\0';

        index++;
    }

    while(!should_stop(buffer, total_bytes_received));

    byte_buffer_t* byte_buffer = malloc(sizeof(byte_buffer_t)); 
    
    if(byte_buffer == NULL) return NULL;

    byte_buffer->buffer = buffer;
    byte_buffer->length = total_bytes_received;

    return byte_buffer;
}

void connection_free(byte_buffer_t* byte_buffer) {
    free(byte_buffer->buffer);
    free(byte_buffer);
}