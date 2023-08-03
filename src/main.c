#include "osapi.h"
#include "user_interface.h"

#include "user_config.h"
#include "server.h"

os_timer_t timer_print;

static void timer_print_cb(void){
    os_printf("I'm working!\n");
}

static void ICACHE_FLASH_ATTR wifi_init(uint8 opmode){
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
        wifi_set_opmode_current(SOFTAP_MODE);
        char ssid[32] = DEVICE_NAME;
        char password[64] = "12345678";
        struct softap_config conf;
        conf.ssid_len = os_strlen(DEVICE_NAME);
        os_memset(&conf.ssid, 0, 32);
        os_memcpy(&conf.ssid, ssid, 32);
        os_memset(&conf.password, 0, 64);
        os_memcpy(&conf.password, password, 64);
        conf.max_connection = 4;
        conf.beacon_interval = 100;
        wifi_softap_set_config_current(&conf);
    }
}

void ICACHE_FLASH_ATTR user_init(void){
    uart_div_modify(0, UART_CLK_FREQ/115200);
    os_delay_us(1000); // wait for clock to stablize

    os_printf("started!\n");
    os_printf("setting wifi");
    wifi_init(STATION_MODE);
    os_printf(" ... done\n");
    os_printf("starting server");
    server_init(80);
    os_printf("... done\n");

    // registering the print timer:
    os_timer_disarm(&timer_print);
    os_timer_setfn(&timer_print, (os_timer_func_t *)timer_print_cb, NULL);
    os_timer_arm(&timer_print, 20000, 1);
}
