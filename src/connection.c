#include <stdio.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdbool.h>
#include <strings.h>
#include "../include/connection.h"

static const char* find_header_end(const char* buffer, size_t length) {
    if (length < 4) return NULL;

    for (size_t i = 0; i <= length - 4; i++) {
        if (buffer[i] == '\r' &&
            buffer[i + 1] == '\n' &&
            buffer[i + 2] == '\r' &&
            buffer[i + 3] == '\n') {
            return buffer + i + 4;
        }
    }

    return NULL;
}

static size_t parse_content_length(const char* buffer, size_t header_length) {
    const char* ptr = buffer;
    const char* end = buffer + header_length;
    const char content_length_name[] = "Content-Length:";
    const size_t content_length_name_len = sizeof(content_length_name) - 1;

    while (ptr < end) {
        const char* line_end = ptr;

        while (line_end + 1 < end && !(line_end[0] == '\r' && line_end[1] == '\n')) {
            line_end++;
        }

        size_t line_len = (size_t)(line_end - ptr);

        if (line_len >= content_length_name_len &&
            strncasecmp(ptr, content_length_name, content_length_name_len) == 0) {
            size_t content_length = 0;
            sscanf(ptr + content_length_name_len, "%zu", &content_length);
            return content_length;
        }

        if (line_end + 1 >= end) break;
        ptr = line_end + 2;
    }

    return 0;
}

bool should_stop(char* buffer, size_t total_bytes_received) {
    const char* body_start = find_header_end(buffer, total_bytes_received);
    if (body_start == NULL) return false;

    size_t header_length = (size_t)(body_start - buffer);
    size_t content_length = parse_content_length(buffer, header_length);
    size_t expected_length = header_length + content_length;

    return total_bytes_received >= expected_length;
}

byte_buffer_t* connection_receive_request(int client_fd) {
    size_t BUFFER_SIZE = 1024;
    char* buffer = NULL;
    char* temp = NULL;
    ssize_t bytes_received;
    size_t total_bytes_received = 0;
    size_t capacity = 0;

    do{
        if (total_bytes_received + BUFFER_SIZE + 1 > capacity) {
            capacity += BUFFER_SIZE;
            temp = realloc(buffer, (capacity + 1) * sizeof(char));

            if (temp == NULL) {
                free(buffer);
                return NULL;
            };

            buffer = temp;
        }


        bytes_received = recv(client_fd, buffer + total_bytes_received, BUFFER_SIZE, 0);

        // currently I'm avoiding complex error handling
        if (bytes_received <= 0) {
            free(buffer);
            return NULL;
        };

        total_bytes_received += bytes_received;

        buffer[total_bytes_received] = '\0';
    }

    while(!should_stop(buffer, total_bytes_received));

    byte_buffer_t* byte_buffer = malloc(sizeof(byte_buffer_t)); 
    
    if(byte_buffer == NULL) return NULL;

    byte_buffer->buffer = buffer;
    byte_buffer->length = total_bytes_received;

    return byte_buffer;
}

void connection_free(byte_buffer_t* byte_buffer) {
    if(byte_buffer == NULL) return;
    free(byte_buffer->buffer);
    free(byte_buffer);
}
