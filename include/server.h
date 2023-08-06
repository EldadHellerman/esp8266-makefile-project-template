#ifndef SERVER_H
#define SERVER_H

#include "espconn.h"
#include "build/files.h"

#define HTTP_SERVER_SUCCESS 0
#define HTTP_SERVER_ERR_MEM 1

enum http_request_types{
    HTTP_GET    = 0x01,
    HTTP_POST   = 0x02,
};

struct http_request{
    enum http_request_types type;
    char *path;
    char *get_variables;
    int content_length;
    char *content;
};

typedef void (* http_callback)(struct espconn *connection, struct http_request http_request);


void ICACHE_FLASH_ATTR server_start(int port);
void ICACHE_FLASH_ATTR server_stop();

int ICACHE_FLASH_ATTR server_register(char *path, enum http_request_types types, char *content_type, file_ptr file, http_callback cb_func);
int ICACHE_FLASH_ATTR server_register_default(char *content_type, file_ptr file, http_callback cb_func);

void ICACHE_FLASH_ATTR server_send_http(struct espconn *conn, int status_code, char *status_text, char *content_type, file_ptr file);

#endif /* SERVER_H */