#include "osapi.h"
#include "user_interface.h"

#include "user_config.h"
#include "server.h"
#include "build/files.h"

os_timer_t timer_print;

static void timer_print_cb(void){
    os_printf("I'm working!\n");
}

static void ICACHE_FLASH_ATTR wifi_init(uint8 opmode, char *ssid, char *password){
    if(opmode == STATION_MODE){
        wifi_set_opmode_current(STATION_MODE);
        wifi_station_dhcpc_set_maxtry(4);
        wifi_station_set_hostname(DEVICE_NAME);
        wifi_station_set_reconnect_policy(true);
        struct station_config conf;
        os_memset(&conf.ssid, 0, 32);
        os_memset(&conf.password, 0, 64);
        os_memcpy(&conf.ssid, ssid, os_strlen(ssid));
        os_memcpy(&conf.password, password, os_strlen(password));
        wifi_station_set_config_current(&conf);
    }else if(opmode == SOFTAP_MODE){
        wifi_set_opmode_current(SOFTAP_MODE);
        char ssid[32] = DEVICE_NAME;
        char password[64] = "12345678";
        struct softap_config conf;
        conf.ssid_len = os_strlen(ssid);
        os_memset(&conf.ssid, 0, 32);
        os_memset(&conf.password, 0, 64);
        os_memcpy(&conf.ssid, ssid, os_strlen(ssid));
        os_memcpy(&conf.password, password, os_strlen(password));
        conf.max_connection = 4;
        conf.beacon_interval = 100;
        wifi_softap_set_config_current(&conf);
    }
}

void http_cb_flash_write(struct espconn *connection, struct http_request request){
    os_printf("flash write path %s with type %d content-length %d data: %s\n", request.path, request.type, request.content_length, request.content);
    server_send_http(connection, 204, "", NULL, NULL);
}

void http_cb_index(struct espconn *connection, struct http_request request){
    os_printf("index path %s with type %d content-length %d data: %s, get variables %s\n", request.path, request.type, request.content_length, request.content, request.get_variables);
}

void ICACHE_FLASH_ATTR user_init(void){
    uart_div_modify(0, UART_CLK_FREQ/115200);
    os_delay_us(1000); // wait for clock to stablize

    os_printf("started!\n");
    os_printf("setting wifi");
    wifi_init(STATION_MODE, "ESP8266-UPGRADE", "12345678");
    os_printf(" ... done\n");
    os_printf("starting server");
    server_register("/", HTTP_GET, "text/html", file_index_html, NULL);
    server_register("/index.html", HTTP_GET, "text/html", file_index_html, http_cb_index);
    server_register("/more/nested.html", HTTP_GET, "text/html", file_more_nested_html, NULL);
    server_register("/favicon.ico", HTTP_GET, "image/png", file_favicon_png, NULL);
    server_register("/flash_write", HTTP_POST, NULL, NULL, http_cb_flash_write);
    server_register_default("text/html", file_404_html, NULL);
    server_start(80);
    os_printf("... done\n");

    // registering the print timer:
    os_timer_disarm(&timer_print);
    os_timer_setfn(&timer_print, (os_timer_func_t *)timer_print_cb, NULL);
    os_timer_arm(&timer_print, 20000, 1);
}
