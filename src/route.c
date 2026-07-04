#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "../include/route.h"

static void route_segments_free(route_segment_t* segments, size_t segment_count) {
    for(size_t i = 0; i < segment_count; i++) {
        free(segments[i].value);
    }

    free(segments);
}

static bool route_add_segment(route_t* route, char* token, size_t* capacity) {
    if(route->segment_count == *capacity) {
        *capacity *= 2;

        route_segment_t* temp = realloc(
            route->segments,
            sizeof(route_segment_t) * (*capacity)
        );

        if(temp == NULL) return false;

        route->segments = temp;
    }

    route_segment_t* segment = &route->segments[route->segment_count];

    segment->type = *token == ':' ? ROUTE_SEG_PARAM : ROUTE_SEG_STATIC;
    segment->value = strdup(*token == ':' ? token + 1 : token);

    if(segment->value == NULL) return false;

    route->segment_count++;

    return true;
}

static bool route_compile_segments(route_t* route, char* path) {
    if(strcmp(path, "/") == 0) {
        return true;
    }

    char* source = strdup(path);
    if(source == NULL) return false;

    size_t capacity = INITIAL_SEGMENT_CAPACITY;
    route->segments = malloc(sizeof(route_segment_t) * capacity);

    if(route->segments == NULL) {
        free(source);
        return false;
    }

    char* token = strtok(source, "/");

    while(token != NULL) {
        if(!route_add_segment(route, token, &capacity)) {
            free(source);
            return false;
        }

        token = strtok(NULL, "/");
    }

    free(source);

    return true;
}

bool route_init(route_t* route, char* method, char* path, route_handler handler) {
    *route = (route_t) {0};

    route->path = strdup(path);
    if(route->path == NULL) goto cleanup;

    route->method = method;
    route->handler = handler;

    if(!route_compile_segments(route, path)) goto cleanup;

    return true;

    cleanup:
        route_free(route);
        return false;
}

void route_free(route_t* route) {
    if(route == NULL) return;

    free(route->path);
    route_segments_free(route->segments, route->segment_count);

    *route = (route_t) {0};
}

route_t* route_find_exact(
    route_t* routes,
    size_t route_count,
    char* path,
    char* method
) {
    for(size_t i = 0; i < route_count; i++) {
        if(strcmp(routes[i].method, method) == 0 && strcmp(routes[i].path, path) == 0) {
            return &routes[i];
        }
    }

    return NULL;
}

static void path_segments_free(char** segments, size_t segment_count) {
    for(size_t i = 0; i < segment_count; i++) {
        free(segments[i]);
    }

    free(segments);
}

static bool path_segments_add(char*** segments, size_t* segment_count, size_t* capacity, char* token) {
    if(*segment_count == *capacity) {
        *capacity *= 2;

        char** temp = realloc(*segments, sizeof(char*) * (*capacity));
        if(temp == NULL) return false;

        *segments = temp;
    }

    (*segments)[*segment_count] = strdup(token);
    if((*segments)[*segment_count] == NULL) return false;

    (*segment_count)++;

    return true;
}

static bool path_split(char* path, char*** segments_out, size_t* segment_count_out) {
    *segments_out = NULL;
    *segment_count_out = 0;

    if(strcmp(path, "/") == 0) {
        return true;
    }

    char* source = strdup(path);
    if(source == NULL) return false;

    size_t capacity = INITIAL_SEGMENT_CAPACITY;
    char** segments = malloc(sizeof(char*) * capacity);

    if(segments == NULL) {
        free(source);
        return false;
    }

    size_t segment_count = 0;
    char* token = strtok(source, "/");

    while(token != NULL) {
        if(!path_segments_add(&segments, &segment_count, &capacity, token)) {
            path_segments_free(segments, segment_count);
            free(source);
            return false;
        }

        token = strtok(NULL, "/");
    }

    free(source);

    *segments_out = segments;
    *segment_count_out = segment_count;

    return true;
}

static bool route_matches_path(
    route_t* route,
    char** path_segments,
    size_t path_segment_count,
    hashmap* params
) {
    if(route->segment_count != path_segment_count) {
        return false;
    }

    for(size_t i = 0; i < route->segment_count; i++) {
        route_segment_t route_segment = route->segments[i];
        char* path_segment = path_segments[i];

        if(route_segment.type == ROUTE_SEG_STATIC) {
            if(strcmp(route_segment.value, path_segment) != 0) {
                return false;
            }
        }

        else {
            hashmap_put(params, route_segment.value, strdup(path_segment));
        }
    }

    return true;
}

route_t* route_find(
    route_t* routes,
    size_t route_count,
    char* path,
    char* method,
    hashmap** params_out
) {
    char** path_segments = NULL;
    size_t path_segment_count = 0;

    if(params_out != NULL) {
        *params_out = NULL;
    }

    route_t* exact_route = route_find_exact(routes, route_count, path, method);

    if(exact_route != NULL) {
        if(params_out != NULL) {
            *params_out = hashmap_create();
            if(*params_out == NULL) return NULL;
        }

        return exact_route;
    }

    if(!path_split(path, &path_segments, &path_segment_count)) {
        return NULL;
    }

    for(size_t i = 0; i < route_count; i++) {
        if(strcmp(routes[i].method, method) != 0) {
            continue;
        }

        hashmap* params = hashmap_create();
        if(params == NULL) goto cleanup;

        if(route_matches_path(&routes[i], path_segments, path_segment_count, params)) {
            if(params_out != NULL) {
                *params_out = params;
            }
            else {
                hashmap_destroy(params);
            }

            path_segments_free(path_segments, path_segment_count);
            return &routes[i];
        }

        hashmap_destroy(params);
    }

    cleanup:
        path_segments_free(path_segments, path_segment_count);

        return NULL;
}
