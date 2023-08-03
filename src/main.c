#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "stdbool.h"
#include "string.h"
#include "mem.h"
#include "espconn.h"

#include "user_config.h"
#include "build/files.h"

#define TASK_SERVER_CLOSE_PRIORITY 0
#define TASK_SERVER_CLOSE_SIG 'a'
#define TASK_SERVER_CLOSE_PARAMETER 'c'

#define TASK_SERVER_SEND_FILE_PRIORITY 0
#define TASK_SERVER_SEND_FILE_SIG 'f'

os_event_t *task_queue;
struct espconn main_esp_conn;
struct espconn *esp_conn_to_close;
esp_tcp main_tcp;


void ICACHE_FLASH_ATTR user_main_server_disconnect(){
	system_os_post(TASK_SERVER_CLOSE_PRIORITY, TASK_SERVER_CLOSE_SIG, TASK_SERVER_CLOSE_PARAMETER);
}

void ICACHE_FLASH_ATTR tcp_main_server_sent_cb_default(void *arg){}

void ICACHE_FLASH_ATTR http_send_header(struct espconn *conn, int status_code, char *status_text, int content_length, char *content_type){
	/* sends an HTTP response header. */
	os_printf("sending http response header ... ");
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
	//fill in request with status code, status text, content-length, and content type:
	int response_header_size = os_snprintf(response_header, response_header_template_max_size, response_header_template, status_code, status_text, content_length, content_type);
	//sending response header:
	sint8 r = espconn_send(conn, (uint8 *)response_header, response_header_size);
	if(r != 0) os_printf("failed! error code: %d\r\n",r);
	else os_printf("sent\r\n");
}

char *http_send_content_data;
int http_send_content_size;
void ICACHE_FLASH_ATTR http_send_content(struct espconn *conn){
	/* sends an HTTP response content. */
	espconn_regist_sentcb(conn, tcp_main_server_sent_cb_default);
	os_printf("sending http response content ... ");
	//sending response content:
	sint8 r = espconn_send(conn, (uint8 *)http_send_content_data, http_send_content_size);
	if(r != 0) os_printf("failed! error code: %d\r\n",r);
	else os_printf("sent\r\n");
}

void ICACHE_FLASH_ATTR tcp_server_send_html(struct espconn *conn, int status_code, char *status_text, char *content_type, char *file, uint32 file_size){
	/* sends a file using as http response. */
	http_send_content_data = file;
	http_send_content_size = file_size;
	espconn_regist_sentcb(conn, (espconn_sent_callback)http_send_content);
	http_send_header(conn, status_code, status_text, file_size, content_type);
}

static bool ICACHE_FLASH_ATTR string_starts_with(char *string, char* start){
	return(os_strncmp(string,start,os_strlen(start)) == 0);
}

static void ICACHE_FLASH_ATTR task_cb(os_event_t *e){
	if(e->sig == TASK_SERVER_CLOSE_SIG && e->par == TASK_SERVER_CLOSE_PARAMETER){
		espconn_disconnect(esp_conn_to_close);
	}
}

void ICACHE_FLASH_ATTR tcp_main_server_recv_cb(void *arg, unsigned char *rawdata, unsigned short length){
	os_printf("tcp recive %d bytes, starts with \"", (int)length);
	unsigned char *i = rawdata;
	while(*i != '\r') os_printf("%c",*i++);
	os_printf("\"\r\n");

	struct espconn *conn = (struct espconn *)arg;
	esp_conn_to_close = conn;
	int index = 0;
	char *data = (char *)rawdata;
	if(string_starts_with(data, "GET ")){
		index += 4;
		data += 4;
		if(string_starts_with(data, "/ ") || string_starts_with(data, "/index.html ")){
			tcp_server_send_html(conn, 200, "OK", "text/html", file_index_html, file_index_html_size);
		}else if(string_starts_with(data, "/404.html ")){
			tcp_server_send_html(conn, 404, "GO TO HELL", "text/html", file_404_html, file_404_html_size);
		}else if(string_starts_with(data, "/more/nested.html ")){
			tcp_server_send_html(conn, 200, "OK", "text/html", file_more_nested_html, file_more_nested_html_size);
		}else{
			os_printf("get request to an non-existing path\r\n");
			tcp_server_send_html(conn, 404, "GO TO HELL", "text/html", file_404_html, file_404_html_size);
		}
	}else{
		os_printf("invalid request (not a GET nor a POST request)\r\n");
		user_main_server_disconnect();
	}
}

void ICACHE_FLASH_ATTR user_main_server_init(uint32 port){
	const int number_of_tasks = 2;
	task_queue = (os_event_t *)os_malloc(sizeof(os_event_t)* number_of_tasks);
	system_os_task(task_cb, TASK_SERVER_CLOSE_PRIORITY, task_queue, number_of_tasks); //close connection

	main_esp_conn.type = ESPCONN_TCP;
	main_esp_conn.state = ESPCONN_NONE;
	main_tcp.local_port = port;
	main_esp_conn.proto.tcp = &main_tcp;
	sint8 r = espconn_accept(&main_esp_conn);
	if(r != 0){
		os_printf("failed to create main server! error code: %d\r\n", r);
		return;
	}
	espconn_regist_recvcb(&main_esp_conn, (espconn_recv_callback)tcp_main_server_recv_cb);
	// espconn_regist_write_finish(&main_esp_conn,(espconn_sent_callback)http_send_content);
	// espconn_regist_sentcb(&main_esp_conn, (espconn_sent_callback)http_send_content);
}

os_timer_t timer_print;

void timer_print_cb(void){
	os_printf("I'm working!\r\n");
}

void ICACHE_FLASH_ATTR wifi_init(uint8 opmode){
	if(opmode == STATION_MODE){
		wifi_set_opmode_current(STATION_MODE);
		wifi_station_dhcpc_set_maxtry(4);
		wifi_station_set_hostname(DEVICE_NAME);
		wifi_station_set_reconnect_policy(true);
		char ssid[32] = "ESP8266-UPGRADE";
		char password[64] = "12345678";
		struct station_config conf;
		os_memset(&conf.ssid, 0, 32);
		os_memset(&conf.password, 0, 64);
		os_memcpy(&conf.ssid, ssid, 32);
		os_memcpy(&conf.password, password, 64);
		wifi_station_set_config_current(&conf);
	}else if(opmode == SOFTAP_MODE){
		wifi_set_opmode_current(STATION_MODE);
		char ssid[32] = DEVICE_NAME;
		char password[64] = "12345678";
		struct softap_config conf;
		os_memset(&conf.ssid, 0, 32);
		os_memset(&conf.password, 0, 64);
		os_memcpy(&conf.ssid, ssid, 32);
		os_memcpy(&conf.password, password, 64);
		wifi_softap_set_config_current(&conf);
	}
}

void ICACHE_FLASH_ATTR user_init(void){
	uart_div_modify(0, UART_CLK_FREQ/115200);
	os_delay_us(1000); //wait for clock to stablize
	
	os_printf("started!\n");
	os_printf("setting wifi");
	wifi_init(STATION_MODE);
	os_printf(" ... done\n");
	os_printf("starting server");
	user_main_server_init(80);
	os_printf("... done\n");

	//registering a timer:
	os_timer_disarm(&timer_print);
	os_timer_setfn(&timer_print, (os_timer_func_t *)timer_print_cb, NULL);
	os_timer_arm(&timer_print, 10000, 1);
}
