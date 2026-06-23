#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "../include/http.h"

void trim_inplace(char* s) {
    if (!s) return;

    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);

    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len-1])) s[--len] = '\0';
}

size_t check_for_body_length(http_header_t** headers, size_t size) {
    http_header_t* header = NULL;
    size_t result = 0;

    for(size_t i = 0; i < size; i++) {
        header = headers[i];

        if(strcmp("Content-Length", header->name) == 0) {
            sscanf(header->value, "%zu", &result);
            return result;
        }
    }

    return 0;
}