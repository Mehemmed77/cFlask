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

bool file_exists(const char* filename) {
    bool flag = false;
    size_t length = snprintf(NULL, 0, "templates/%s", filename) + 1;

    char* path = malloc(sizeof(char) * length);
    
    if(path == NULL) {
        perror("Failed to allocate memory for path");
        goto cleanup;
    }

    snprintf(path, length, "templates/%s", filename);

    FILE *f = fopen(path, "r");

    if(f == NULL) {
        perror("NO");
        goto cleanup;
    }

    else{
        printf("YES");
    }

    fflush(stdout);

    cleanup:
        free(path);
        return flag;
}

http_response_t* render_template(const char* filename) {
    file_exists(filename);
    return NULL;
}
