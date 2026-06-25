CC = gcc

CFLAGS = -Wall -Wextra -pedantic -g -fsanitize=address
INCLUDES = -Iinclude

SERVER = compiled/server

HTTP_SRC = \
	src/http_request.c \
	src/http_request_parser.c \
	src/http_response.c \
	src/http_bounds.c \
	src/http_utils.c \
	src/app.c

SERVER_SRC = \
	src/server.c \
	src/connection.c

SRC = $(SERVER_SRC) $(HTTP_SRC)

all: $(SERVER)

$(SERVER): $(SRC)
	mkdir -p compiled
	$(CC) $(CFLAGS) $(INCLUDES) $(SRC) -o $(SERVER)

clean:
	rm -f $(SERVER)

.PHONY: all clean