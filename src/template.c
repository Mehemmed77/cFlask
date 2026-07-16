#include "../include/template.h"
#include "../include/constants.h"
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

void print_filename_errors(enum filename_errors fe) {
    if(fe == FILENAME_NULL) {
        perror("filename cannot be NULL");
    }
    
    else if(fe == FILENAME_EMPTY) {
        perror("filename cannot be empty");
    }

    else if(fe == FILENAME_ABSOLUTE_PATH) {
        perror("absolute paths are not allowed");
    }

    else if(fe == FILENAME_TRAVERSAL) {
        perror("path traversing is not allowed");
    }

    else {
        perror("backlashing is not allowed for filenames");
    }
}

bool validate_filename(const char* filename) {
    if(filename == NULL) {
        print_filename_errors(FILENAME_NULL);
        return false;
    };

    if(filename[0] == '\0') {
        print_filename_errors(FILENAME_EMPTY);
        return false;
    }

    if(filename[0] == '/') {
        print_filename_errors(FILENAME_ABSOLUTE_PATH);
        return false;
    }

    if(strstr(filename, "..") != NULL) {
        print_filename_errors(FILENAME_TRAVERSAL);
        return false;
    }

    if(strchr(filename, '\\') != NULL) {
        print_filename_errors(FILENAME_BACKSLASH);
        return false;
    }

    return true;
}

http_response_t* render_template(const char* filename) {
    if(!validate_filename(filename)) return NULL;

    http_response_t* response = NULL;

    size_t length = snprintf(NULL, 0, "templates/%s", filename) + 1;

    char* path = malloc(sizeof(char) * length);
    
    if(path == NULL) {
        perror("Failed to allocate memory for path");
        goto cleanup;
    }

    snprintf(path, length, "templates/%s", filename);

    FILE* f = fopen(path, "rb");

    if(f == NULL) {
        response = http_not_found_response("Template not found");
        goto cleanup;
    }

    size_t capacity = 4096;
    size_t byte_length = 0;
    char* buffer = malloc(capacity);

    while(1) {
        if(byte_length + 1024 + 1 > capacity) {
            capacity *= 2;
            buffer = realloc(buffer, capacity);
        }

        size_t n = fread(buffer + byte_length, 1, 1024, f);
        byte_length += n;

        if(n < 1024) {
            if(ferror(f)) {

            }

            break;
        }
    }

    buffer[byte_length] = '\0';

    response = http_html_response(buffer);

    cleanup:
        free(path);
        if(f != NULL) fclose(f);
        return response;
}
