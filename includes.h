#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <poll.h>
#include <fcntl.h>
#include <sys/stat.h>

#define PORT_NO             5555
#define MAX_CLIENT          10
#define BUFFER_SIZE         4096
#define WWW_ROOT            "./www"
#define HTTP_ERR_STR        "HTTP/1.1 500 Internal Server Error\r\n" \
                            "Content-Type: text/html\r\n" \
                            "Connection: close\r\n\r\n" \
                            "<html><body><h1>500 Internal Server Error</h1></body></html>"

#define HTTP_SUCCESS_RESP   "HTTP/1.1 200 OK\r\n"\
                            "Content-Type: %s\r\n"\
                            "Content-Length: %ld\r\n"\
                            "Connection: close\r\n\r\n"

void vRunHttpServer();

