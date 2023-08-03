CC = xtensa-lx106-elf-gcc
AR = xtensa-lx106-elf-ar
LD = xtensa-lx106-elf-ld
OBJ_COPY = xtensa-lx106-elf-objcopy
OBJ_DUMP = xtensa-lx106-elf-objdump

GCC_INCLUDE_DIR = "/mnt/d/Hobbies/programing/esp8266/espressif/xtensa-lx106-elf/xtensa-lx106-elf/include/"
# why isnt there a string.h in xtensa-lx106-elf installed with apt-get?
# /usr/lib/gcc/xtensa-lx106-elf/10.3.0/include

OUTPUT_FILE_NAME = app
SRC_DIR = src
INCLUDE_DIR = include
FILES_DIR = files
LD_SCRIPTS_DIR = linker_scripts
SDK_DIR = /mnt/d/Hobbies/programing/esp8266/espressif/ESP8266_NONOS_SDK
SDK_INCLUDE_DIR = $(SDK_DIR)/include/
SDK_LIB_DIR = $(SDK_DIR)/lib
SDK_LINKER_SCRIPTS_DIR = $(SDK_DIR)/ld

#single app:
LINKER_SCRIPT = $(LD_SCRIPTS_DIR)/app-single-1024kB.ld
BUILD_DIR = build

#OTA - two apps:
# APP_NUMBER = 1
# LINKER_SCRIPT = $(LD_SCRIPTS_DIR)/app-$(APP_NUMBER)-512kB.ld
# BUILD_DIR = build/ota-app-$(APP_NUMBER)

FILES = index.html 404.html more/nested.html
FILES_OBJECTS = $(patsubst %,$(BUILD_DIR)/file_%.o,$(subst /,_,$(subst .,_,$(FILES))))

_OBJ = main.o
OBJ = $(patsubst %,$(BUILD_DIR)/%,$(_OBJ)) $(FILES_OBJECTS)

OUTPUT_FILE = $(BUILD_DIR)/$(OUTPUT_FILE_NAME)

CFLAGS = -c -I$(INCLUDE_DIR) -I$(SDK_INCLUDE_DIR) -I$(GCC_INCLUDE_DIR) -mlongcalls -DICACHE_FLASH -Wall -std=c99 #-Werror

LD_FLAGS = -L$(SDK_LIB_DIR) --start-group -lc -lgcc -lhal -lphy -lpp -lnet80211 -llwip -lwpa -lcrypto -lmain -ljson -lupgrade -lssl -lpwm -lsmartconfig --end-group
# LD_FLAGS += --nostdlib -nodefaultlibs --no-check-sections --gc-sections -static
LD_FLAGS += --nostdlib --no-check-sections --gc-sections -static
LD_FLAGS += -T$(LINKER_SCRIPT) -Map $(OUTPUT_FILE).map
LD_FLAGS += -u call_user_start

.PHONY: build clean upload fresh files

build: $(OUTPUT_FILE)-0x00000.bin dissasembly
	@echo done!

$(OUTPUT_FILE)-0x00000.bin: $(OUTPUT_FILE).elf	
	@echo turning $^ to binary
	@esptool --chip esp8266 elf2image --flash_size 1MB --flash_freq 40m --flash_mode dout $^ > /dev/null
#	esptool image_info $(BUILD_DIR)/main.elf-0x00000.bin

dissasembly: $(OUTPUT_FILE).elf
	@echo dissasembling
	@$(OBJ_DUMP) -t $^ > $(BUILD_DIR)/dissasembly-sections.txt
	@$(OBJ_DUMP) -h $^ > $(BUILD_DIR)/dissasembly-headers.txt
	@$(OBJ_DUMP) -d $^ > $(BUILD_DIR)/dissasembly.txt

#extracting app_partition.o from libmain.a since from some reason ld doesnt find system_partition_table_regist()
$(BUILD_DIR)/app_partition.o: $(SDK_DIR)/lib/libmain.a
	@echo extracting app_partition.o from libmain.a
	@mkdir -p $(BUILD_DIR)/temp/
	@cp $< $(BUILD_DIR)/temp/libmain.a
	@cd $(BUILD_DIR)/temp/; $(AR) x libmain.a
	@cd ../..
	@cp $(BUILD_DIR)/temp/app_partition.o $(BUILD_DIR)/app_partition.o
	@rm -f -r $(BUILD_DIR)/temp
	
$(OUTPUT_FILE).elf: $(OBJ) $(BUILD_DIR)/app_partition.o $(FILES_OBJECTS)
	@echo linking
	@$(LD) $(LD_FLAGS) -o $@ $^

$(BUILD_DIR)/main.o: $(SRC_DIR)/main.c $(INCLUDE_DIR)/user_config.h
	@echo compiling $@
	@$(CC) $(CFLAGS) -o $@ $<

$(FILES_OBJECTS) files:
	@python3 generate_files_ld_script_and_header.py $(LD_SCRIPTS_DIR) $(INCLUDE_DIR) $(BUILD_DIR) $(FILES_DIR) $(OBJ_COPY) $(FILES)

clean:
	rm -f -r $(BUILD_DIR)/*
	clear

fresh: clean build

both:
	make APP_NUMBER=1
	make APP_NUMBER=2

# serial port doens't work for me now in WSL, although it did back in 2018.
# BAUDRATE = 115200
# PORT = COM7
#
#upload: $(BUILD_DIR)/$(OUTPUT_FILE).elf-0x00000.bin $(BUILD_DIR)/$(OUTPUT_FILE).elf-0x10000.bin
#   sudo -S chmod 666 /dev/tty8
#	./tty_access.sh
#	esptool.py --port /dev/ttyS8 --baud $(BAUDRATE) write_flash 0 $(BUILD_DIR)/$(OUTPUT_FILE).elf-0x00000.bin 0x10000 $(BUILD_DIR)/$(OUTPUT_FILE).elf-0x10000.bin