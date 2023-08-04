clear
set COM_PORT=COM7
set BAUD_RATE=921600

@echo flash addresses should be adjusted before use!
@if "%1%" == "" (
    @REM default upload type can be choosen here
    set type=single
) else (
    set type=%1%
)

@if "%type%" == "ota_full" (
    @echo programing OTA app
    @esptool --chip esp8266 --port %COM_PORT% --baud %BAUD_RATE% write_flash --flash_size 1MB --flash_freq 40m --flash_mode dout --verify ^
        0x0000 ..\..\espressif\ESP8266_NONOS_SDK\bin\boot_V1.7.bin ^
        0x01000 build/ota-app-1/app.elf-0x00000.bin ^
        0x11000 build/ota-app-1/app.elf-0x11000.bin ^
        0x81000 build/ota-app-2/app.elf-0x00000.bin ^
        0x91000 build/ota-app-2/app.elf-0x91000.bin ^
        0xFB000 ..\..\espressif\ESP8266_NONOS_SDK\bin\blank.bin ^
        0xFC000 ..\..\espressif\ESP8266_NONOS_SDK\bin\esp_init_data_default_v08.bin ^
        0xFD000 ..\..\espressif\ESP8266_NONOS_SDK\bin\blank.bin ^
        0xFE000 ..\..\espressif\ESP8266_NONOS_SDK\bin\blank.bin ^
        0xFF000 ..\..\espressif\ESP8266_NONOS_SDK\bin\blank.bin
        @echo done.
) else if "%type%" == "ota" (
    @echo programing standalone app
    @esptool --chip esp8266 --port %COM_PORT% --baud %BAUD_RATE% write_flash --flash_size 1MB --flash_freq 40m --flash_mode dout --verify ^
        0x00000 build/app.elf-0x00000.bin ^
        0x10000 build/app.elf-0x10000.bin
    @echo done.
) else if "%type%" == "single_full" (
    @echo programing standalone app
    @esptool --chip esp8266 --port %COM_PORT% --baud %BAUD_RATE% write_flash --flash_size 1MB --flash_freq 40m --flash_mode dout --verify ^
        0x00000 build/app.elf-0x00000.bin ^
        0x10000 build/app.elf-0x10000.bin ^
        0xFB000 ..\..\espressif\ESP8266_NONOS_SDK\bin\blank.bin ^
        0xFC000 ..\..\espressif\ESP8266_NONOS_SDK\bin\esp_init_data_default_v08.bin ^
        0xFD000 ..\..\espressif\ESP8266_NONOS_SDK\bin\blank.bin ^
        0xFE000 ..\..\espressif\ESP8266_NONOS_SDK\bin\blank.bin ^
        0xFF000 ..\..\espressif\ESP8266_NONOS_SDK\bin\blank.bin
    @echo done.
) else if "%type%" == "single" (
    @echo programing standalone app
    @esptool --chip esp8266 --port %COM_PORT% --baud %BAUD_RATE% write_flash --flash_size 1MB --flash_freq 40m --flash_mode dout --verify ^
        0x00000 build/app.elf-0x00000.bin ^
        0x10000 build/app.elf-0x10000.bin
    @echo done.
) else (
    @echo usage: upload [single / ota]    
)
@REM @pause