# ESP8266 makefile project template
I'm building this from WSL and uploading to the chip from windows.

# Setup
To set up the build environment:
1. Clone [ESP8266_NONOS_SDK](https://github.com/espressif/ESP8266_NONOS_SDK) (works with release V3.0.5). This is the newer version of the SDK, with some changes over the older one.
Also, comment out the line "#include <string.h>" from file "include/osapi.h".
2. Install xtensa-lx106-elf-gcc, esptool and make on WSL. These are used to build the proeject and upload it to the chip.
3. Run make to build the binaries.
4. Use the upload script on windows to upload to the chip.
```
git clone https://github.com/espressif/ESP8266_NONOS_SDK
wsl sudo apt-get install make esptool gcc-xtensa-lx106
wsl make
upload
```