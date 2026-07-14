#ifndef CONSTANSTS_H
#define CONSTANSTS_H

#define NOT_FOUND 404
#define NOT_FOUND_TEXT "Not Found"
#define OK 200
#define OK_TEXT "OK"

enum filename_errors {
    FILENAME_NULL,
    FILENAME_EMPTY,
    FILENAME_ABSOLUTE_PATH,
    FILENAME_TRAVERSAL,
    FILENAME_BACKSLASH,
};

#define TEXT_PLAIN "text/plain"
#define TEXT_HTML "text/html"
#define APPLICATION_JSON "application/json"

#endif
