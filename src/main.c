#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "stdbool.h"
#include "string.h"
#include "mem.h"

#include "user_config.h"

static const partition_item_t partition_table[] = {
    // { SYSTEM_PARTITION_BOOTLOADER, 	0x00000, 0x00000},
    // { SYSTEM_PARTITION_OTA_1, 	0x19000, 0x68000},
    // { SYSTEM_PARTITION_OTA_2, 	0x91000, 0x68000},
    { SYSTEM_PARTITION_CUSTOMER_BEGIN + 1, 	0x00000, 0x10000},
    { SYSTEM_PARTITION_CUSTOMER_BEGIN + 2, 0x10000, 0xEB000},
    { SYSTEM_PARTITION_RF_CAL, 0xFB000, 0x1000},
    { SYSTEM_PARTITION_PHY_DATA, 0xFC000, 0x1000},
    { SYSTEM_PARTITION_SYSTEM_PARAMETER, 0xFD000, 0x3000},
};

void user_pre_init(void){
    if(!system_partition_table_regist(partition_table, sizeof(partition_table)/sizeof(partition_table[0]), FLASH_SIZE_8M_MAP_512_512)){
		os_printf("system_partition_table_regist fail\r\n");
		while(1);
	}
}
void ICACHE_FLASH_ATTR user_rf_pre_init(void){}

void user_spi_flash_dio_to_qio_pre_init(void){}

os_timer_t timer_main;
extern char index_html_start, index_html_end;

void main(void){
	os_printf("I'm working!\r\n");
	os_printf("file start %p end %p size %d!\r\n", &index_html_start, &index_html_end, ((&index_html_end)-(&index_html_start)));
}

void ICACHE_FLASH_ATTR user_init(void){
	uart_div_modify(0,UART_CLK_FREQ/115200); //uart_init(uart0_br, uart1_br); //why am i not using that?
	os_delay_us(1000);
	
	os_printf("\r\nstarted!\r\n");
	os_printf("setting wifi:");

	char ssid[32] = DEVICE_NAME;
	//char password[64] = "iceland2018LEDS";
	char password[64] = "12345678";
	struct softap_config softap_conf;
	os_memset(softap_conf.ssid, 0, 32); os_memset(softap_conf.password, 0, 64);
	os_memcpy(&softap_conf.ssid, ssid, 32); os_memcpy(&softap_conf.password, password, 64);

	wifi_set_opmode_current(SOFTAP_MODE);
	wifi_softap_set_config_current(&softap_conf);
	os_printf("... set\r\n");
	os_timer_disarm(&timer_main);
	os_timer_setfn(&timer_main, (os_timer_func_t *)main, NULL);
	os_timer_arm(&timer_main, 1000, 1);
}
