#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"

#include "build/files.h"
#include "server.h"

struct espconn server_conn;
esp_tcp server_conn_tcp;
char *server_send_http_content_data;
uint16 server_send_http_content_size;

static void ICACHE_FLASH_ATTR server_cb_sent(struct espconn *conn);

static bool ICACHE_FLASH_ATTR string_starts_with(char *string, char* start){
    return(os_strncmp(string, start, os_strlen(start)) == 0);
}

static void ICACHE_FLASH_ATTR server_send_http_header(struct espconn *conn, int status_code, char *status_text, uint16 content_length, char *content_type){
    /* sends an HTTP response header. */
    os_printf("server: sending http header ... ");
    char response_header_template[] =
        "HTTP/1.1 %d %s\r\n"
        "Content-Length: %d \r\n"
        "Content-Type: %s\r\n"
        "Connection: Closed\r\n"
        "\r\n";
    // create a temporary buffer with enough space for http header, and the file itself
    // 40 is added because status text, content-length and content-type have variable size
    int response_header_template_max_size = (sizeof(response_header_template) / sizeof(char)) + 40;
    char response_header[response_header_template_max_size];
    // fill in request with status code, status text, content-length, and content type:
    int response_header_size = os_snprintf(response_header, response_header_template_max_size, response_header_template, status_code, status_text, content_length, content_type);
    // sending response header:
    sint8 r = espconn_send(conn, (uint8 *)response_header, response_header_size);
    if(r != 0) os_printf("failed! error code: %d\n",r);
    else os_printf("sent\n");
}

static void ICACHE_FLASH_ATTR server_send_http_content(struct espconn *conn){
    /* sends an HTTP response content. */
    espconn_regist_sentcb(conn, (espconn_sent_callback)server_cb_sent);
    os_printf("server: sending http content ... ");
    // sending response content:
    sint8 r = espconn_send(conn, (uint8 *)server_send_http_content_data, server_send_http_content_size);
    if(r != 0) os_printf("failed! error code: %d\n",r);
    else os_printf("sent\n");
}

static void ICACHE_FLASH_ATTR server_send_http(struct espconn *conn, int status_code, char *status_text, char *content_type, char *file, uint16 file_size){
    /* sends a file using as http response. */
    server_send_http_content_data = file;
    server_send_http_content_size = file_size;
    espconn_regist_sentcb(conn, (espconn_sent_callback)server_send_http_content);
    server_send_http_header(conn, status_code, status_text, file_size, content_type);
}

// callback functions:

static void ICACHE_FLASH_ATTR server_cb_recive(struct espconn *conn, char *data, uint16 length){
    os_printf("server: cb_recive - %d bytes \"", (int)length);
    char *i = data;
    while(*i != '\r') os_printf("%c",*i++);
    os_printf("[...]\"\n");
    
    if(string_starts_with(data, "GET ")){
        data += 4;
        if(string_starts_with(data, "/ ") || string_starts_with(data, "/index.html ")){
            server_send_http(conn, 200, "OK", "text/html", file_index_html, file_index_html_size);
        }else if(string_starts_with(data, "/404.html ")){
            server_send_http(conn, 404, "GO TO HELL", "text/html", file_404_html, file_404_html_size);
        }else if(string_starts_with(data, "/more/nested.html ")){
            server_send_http(conn, 200, "OK", "text/html", file_more_nested_html, file_more_nested_html_size);
        }else{
            os_printf("server: GET request to an non-existing path\n");
            server_send_http(conn, 404, "GO TO HELL", "text/html", file_404_html, file_404_html_size);
        }
    }else{
        os_printf("server: invalid request (not a GET request)\n");
    }
}

static void ICACHE_FLASH_ATTR server_cb_sent(struct espconn *conn){
    os_printf("server: cb_sent\n");
}

static void ICACHE_FLASH_ATTR server_cb_disconnect(struct espconn *conn){
    os_printf("server: cb_disconnect\n");
}

static void ICACHE_FLASH_ATTR server_cb_reconnect(struct espconn *conn){
    os_printf("server: cb_reconnect\n");
}

static void ICACHE_FLASH_ATTR server_cb_connect(struct espconn *conn){
    os_printf("server: cb_connect\n");
    espconn_regist_recvcb(conn, (espconn_recv_callback)server_cb_recive);
    espconn_regist_sentcb(conn, (espconn_sent_callback)server_cb_sent);
    espconn_regist_disconcb(conn, (espconn_connect_callback)server_cb_disconnect);
}

void ICACHE_FLASH_ATTR server_init(int port){
    server_conn.type = ESPCONN_TCP;
    server_conn.state = ESPCONN_NONE;
    server_conn_tcp.local_port = port;
    server_conn.proto.tcp = &server_conn_tcp;
    
    /* reconnect and connect are registered here,
    recive, sent, and disconnect are registered inside server_cb_connect().
    see Apendix A of "ESP8266 Non-OS SDK V4". */
    espconn_regist_reconcb(&server_conn, (espconn_reconnect_callback)server_cb_reconnect);
    espconn_regist_connectcb(&server_conn, (espconn_connect_callback)server_cb_connect);
    sint8 r = espconn_accept(&server_conn);
    if(r != 0) os_printf("failed to create main server! error code: %d\n", r);
}
