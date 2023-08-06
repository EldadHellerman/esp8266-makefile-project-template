#include "osapi.h"
#include "user_interface.h"
#include "espconn.h"
#include "mem.h"

#include "build/files.h"
#include "server.h"

bool debug = false;

struct path_actions{
    char *path;
    enum http_request_types request_types;
    char *content_type;
    file_ptr file;
    http_callback callback;
};

enum http_parser_state{
    WAITING_FOR_REQUEST,
    READING_TYPE,
    READING_PATH,
    READING_HEADERS,
    READING_CONTENT,
    DONE
};

struct http_request_parser{
    struct http_request request;
    enum http_parser_state state; //current phase inrequest parsing
    int content_bytes_read; //bytes read from content
} parser;

struct espconn server_conn;
esp_tcp server_conn_tcp;
char *server_send_http_content_data;
uint16 server_send_http_content_size;
int number_of_actions = 0;
struct path_actions *actions = NULL;
struct path_actions actions_default;

static void ICACHE_FLASH_ATTR server_cb_sent(struct espconn *conn);

static int ICACHE_FLASH_ATTR atoi(char *string){
    int result = 0;
    while(true){
        char c = *string++;
        if('0' <= c && c <= '9') result = (result * 10) + c - '0'; else break;
    }
    return result;
}

static bool ICACHE_FLASH_ATTR string_starts_with(char *string, char* start){
    return(os_strncmp(string, start, os_strlen(start)) == 0);
}

static void ICACHE_FLASH_ATTR server_send_http_header(struct espconn *conn, int status_code, char *status_text, uint16 content_length, char *content_type){
    /* sends an HTTP response header. */
    if(debug) os_printf("server: sending http header ... ");
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
    if(r != 0){ if(debug) os_printf("failed! error code: %d\n",r);}
    else if(debug) os_printf("sent\n");
}

static void ICACHE_FLASH_ATTR server_send_http_content(struct espconn *conn){
    /* sends an HTTP response content. */
    espconn_regist_sentcb(conn, (espconn_sent_callback)server_cb_sent);
    if(debug) os_printf("server: sending http content ... ");
    // sending response content:
    sint8 r = espconn_send(conn, (uint8 *)server_send_http_content_data, server_send_http_content_size);
    if(r != 0){ if(debug) os_printf("failed! error code: %d\n",r);}
    else if(debug) os_printf("sent\n");
}

void ICACHE_FLASH_ATTR server_send_http(struct espconn *conn, int status_code, char *status_text, char *content_type, file_ptr file){
    /* sends a file using as http response. */
    if(file != NULL){
        server_send_http_content_data = file;
        server_send_http_content_size = file_size(file);
        espconn_regist_sentcb(conn, (espconn_sent_callback)server_send_http_content);
        server_send_http_header(conn, status_code, status_text, server_send_http_content_size, content_type);
    }else{
        server_send_http_header(conn, status_code, status_text, 0, content_type);
    }
}

// callback functions:

static void ICACHE_FLASH_ATTR server_cb_receive(struct espconn *conn, char *data, uint16 length){
    if(debug) os_printf("server: cb_receive - %d bytes\n", (int)length);
    char *parser_index = data;
    char *data_end = data + length;
    while(parser_index != data_end){
        if(parser.state == WAITING_FOR_REQUEST){
            // recieved a new request - free resources used by old request:
            parser.request.type = 0;
            parser.request.content_length = 0;
            os_free(parser.request.path);
            parser.request.path = NULL;
            os_free(parser.request.content);
            parser.request.content = NULL;
            os_free(parser.request.get_variables);
            parser.request.get_variables = NULL;
            parser.content_bytes_read = 0;
            parser.state = READING_TYPE;
        }else if(parser.state == READING_TYPE){
            if(string_starts_with(parser_index, "GET ")){ //assuming data length is at least 4 bytes...
                parser.request.type = HTTP_GET;
                parser_index += 4;
                parser.state = READING_PATH;
            }else if(string_starts_with(parser_index, "POST ")){
                parser.request.type = HTTP_POST;
                parser_index += 5;
                parser.state = READING_PATH;
            }
            // default - advance one letter, to prevent infinite loops from invalid data:
            parser_index += 1;
        }else if(parser.state == READING_PATH){
            int path_length = 0;
            while(*(parser_index + path_length) != ' ' && *(parser_index + path_length) != '?' && (parser_index + path_length) != data_end) path_length++;
            parser.request.path = os_malloc(path_length+2);
            parser.request.path[0] = '/';
            os_memcpy(parser.request.path + 1, parser_index, path_length);
            parser.request.path[path_length+1] = '\0';
            parser_index += path_length;
            int get_variables_length = 0;
            if(*parser_index == '?'){ //read get variables
                while(*(parser_index + get_variables_length) != ' ' && (parser_index + get_variables_length) != data_end) get_variables_length++;
                parser.request.get_variables = os_malloc(get_variables_length);
                os_memcpy(parser.request.get_variables, parser_index+1, get_variables_length-1);
                parser.request.get_variables[get_variables_length-1] = '\0';
                parser_index += get_variables_length;
            }
            parser_index += get_variables_length;
            while(!string_starts_with(parser_index, "\r\n") && parser_index != data_end) parser_index++; //read until end of line
            parser_index += 2;
            parser.state = READING_HEADERS;
        }else if(parser.state == READING_HEADERS){
            if(string_starts_with(parser_index, "\r\n")){ // read end of headers
                parser_index += 2;
                if(parser.request.content_length == 0){
                    parser.state = DONE;
                }else{
                    parser.state = READING_CONTENT;
                }
            }else if(string_starts_with(parser_index, "Content-Length: ")){ //get content-length header
                parser_index += 16;
                parser.request.content_length = atoi(parser_index);
                while(!string_starts_with(parser_index, "\r\n") && parser_index != data_end) parser_index++; //read until end of line
                parser_index += 2;
            }else{ // here different http header can be read (content-encoding or something...)
                while(!string_starts_with(parser_index, "\r\n") && parser_index != data_end) parser_index++; //read until end of line
                parser_index += 2;
            }
        }else if(parser.state == READING_CONTENT){
            if(debug) os_printf("server: reading content ...");
            if(parser.request.content == NULL){
                parser.request.content = os_malloc(parser.request.content_length);
            }
            char *dest = parser.request.content + parser.content_bytes_read;
            int number_of_bytes = data_end - parser_index;
            if(debug) os_printf("server: reading %d bytes (read %d / %d bytes).\n", number_of_bytes, parser.content_bytes_read, parser.request.content_length);
            os_memcpy(dest, parser_index, number_of_bytes);
            parser.content_bytes_read += number_of_bytes;
            parser_index += number_of_bytes;
            if(parser.content_bytes_read == parser.request.content_length){
                parser.state = DONE;
            }
        }else if(parser.state == DONE){
            os_printf("server: error! request is over yet there's more data!\n");
            // this could happen and that why request actions should take place inside the
            // loop, because the rest of the data may be from another request..
        }
    }
    
    if(parser.state == DONE){ // execute registered actions
        bool action_taken = false;
        for(int action_number = 0; action_number < number_of_actions; action_number++){
            struct path_actions *action = &actions[action_number];
            if((os_strcmp(action->path, parser.request.path) == 0) && (action->request_types & parser.request.type)){
                action_taken = true;
                if(action->file != NULL) server_send_http(conn, 200, "OK", action->content_type, action->file);
                if(action->callback != NULL) action->callback(conn, parser.request);
            }
        }
        if(!action_taken){
            if(debug) os_printf("server: no registered action, using default actions\n");
            if(actions_default.file != NULL) server_send_http(conn, 404, "GO TO HELL", actions_default.content_type, actions_default.file);
            if(actions_default.callback != NULL) actions_default.callback(conn, parser.request);
        }
        parser.state = WAITING_FOR_REQUEST;
    }
}

static void ICACHE_FLASH_ATTR server_cb_sent(struct espconn *conn){
    if(debug) os_printf("server: cb_sent\n");
}

static void ICACHE_FLASH_ATTR server_cb_disconnect(struct espconn *conn){
    if(debug) os_printf("server: cb_disconnect\n");
}

static void ICACHE_FLASH_ATTR server_cb_reconnect(struct espconn *conn){
    if(debug) os_printf("server: cb_reconnect\n");
}

static void ICACHE_FLASH_ATTR server_cb_connect(struct espconn *conn){
    if(debug) os_printf("server: cb_connect\n");
    espconn_regist_recvcb(conn, (espconn_recv_callback)server_cb_receive);
    espconn_regist_sentcb(conn, (espconn_sent_callback)server_cb_sent);
    espconn_regist_disconcb(conn, (espconn_connect_callback)server_cb_disconnect);
}

void ICACHE_FLASH_ATTR server_start(int port){
    server_conn.type = ESPCONN_TCP;
    server_conn.state = ESPCONN_NONE;
    server_conn_tcp.local_port = port;
    server_conn.proto.tcp = &server_conn_tcp;
    
    parser.state = WAITING_FOR_REQUEST;

    /* reconnect and connect are registered here,
    receive, sent, and disconnect are registered inside server_cb_connect().
    see Apendix A of "ESP8266 Non-OS SDK V4". */
    espconn_regist_reconcb(&server_conn, (espconn_reconnect_callback)server_cb_reconnect);
    espconn_regist_connectcb(&server_conn, (espconn_connect_callback)server_cb_connect);
    sint8 r = espconn_accept(&server_conn);
    if(r != 0){ if(debug) os_printf("server: failed to create server! error code: %d\n", r);}
}

void ICACHE_FLASH_ATTR server_stop(){
    sint8 r = espconn_disconnect(&server_conn);
    /*
    TODO release all memory resources used:
    - current http request
    - actions registered
    */
    if(r != 0){ if(debug) os_printf("server: failed to stop server! error code: %d\n", r);}
}

int ICACHE_FLASH_ATTR server_register(char *path, enum http_request_types types, char *content_type, file_ptr file, http_callback cb_func){
    actions = (struct path_actions *)os_realloc(actions, sizeof(struct path_actions)*(number_of_actions + 1));
    if(actions == NULL) return HTTP_SERVER_ERR_MEM;
    actions[number_of_actions].path = path;
    actions[number_of_actions].request_types = types;
    actions[number_of_actions].content_type = content_type;
    actions[number_of_actions].file = file;
    actions[number_of_actions].callback = cb_func;
    number_of_actions++;
    return HTTP_SERVER_SUCCESS;
}

int ICACHE_FLASH_ATTR server_register_default(char *content_type, file_ptr file, http_callback cb_func){
    actions_default.request_types = 0;
    actions_default.path = NULL;
    actions_default.content_type = NULL;
    actions_default.file = file;
    actions_default.callback = cb_func;
    return HTTP_SERVER_SUCCESS;
}
