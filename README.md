# ESP8266 makefile project template
I'm building this with WSL (through VS code) and uploading to the chip from windows.

# Setup
To set up the build environment:
1. Clone [ESP8266_NONOS_SDK](https://github.com/espressif/ESP8266_NONOS_SDK) (works with release V3.0.5). This is the newer version of the SDK, with some changes over the older one.
2. Clone [esptool](https://github.com/espressif/esptool). This is used to upload the code to the chip.
3. Install xtensa-lx106-elf-gcc on WSL (as well as Make and maybe other dependencies).
```
sudo apt-get install make esptool gcc-xtensa-lx106
```
4. Run make to build the binaries.
5. Use the upload script on windows to upload to the chip.

Due to a missing "string.h", I'm currently also including the include dir of [espressif xtensa-lx106-elf toolchain (2020r3)](https://dl.espressif.com/dl/xtensa-lx106-elf-gcc8_4_0-esp-2020r3-linux-amd64.tar.gz).
