#include "user_interface.h"
#include "osapi.h"

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

void ICACHE_FLASH_ATTR user_pre_init(void){
    if(!system_partition_table_regist(partition_table, sizeof(partition_table)/sizeof(partition_table[0]), FLASH_SIZE_8M_MAP_512_512)){
        os_printf("init: system_partition_table_regist() failed!\n");
        while(1);
    }
}

void ICACHE_FLASH_ATTR user_rf_pre_init(void){}

void ICACHE_FLASH_ATTR user_spi_flash_dio_to_qio_pre_init(void){}