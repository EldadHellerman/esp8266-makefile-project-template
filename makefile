CC = xtensa-lx106-elf-gcc
AR = xtensa-lx106-elf-ar
LD = xtensa-lx106-elf-ld
OBJ_COPY = xtensa-lx106-elf-objcopy
OBJ_DUMP = xtensa-lx106-elf-objdump

DIR_SDK = /mnt/d/Hobbies/programing/esp8266/espressif/ESP8266_NONOS_SDK
DIR_SRC = src
DIR_INCLUDE = include
DIR_FILES = files
DIR_LD_SCRIPTS = linker_scripts
DIR_BUILD = build
OUTPUT_FILE = $(DIR_BUILD)/app
LINKER_SCRIPT = $(DIR_LD_SCRIPTS)/app-single-1024kB.ld
# OTA - two apps:
# APP_NUMBER = 1
# LINKER_SCRIPT = $(DIR_LD_SCRIPTS)/app-$(APP_NUMBER)-512kB.ld
# DIR_BUILD = build/ota-app-$(APP_NUMBER)
_OBJ = init.o main.o server.o
FILES = index.html 404.html more/nested.html favicon.png

OBJ = $(patsubst %,$(DIR_BUILD)/%,$(_OBJ)) $(FILES_OBJECTS)
FILES_OBJECTS = $(patsubst %,$(DIR_BUILD)/file_%.o,$(subst /,_,$(subst .,_,$(FILES))))

CFLAGS = -c -I$(DIR_INCLUDE) -I. -I$(DIR_SDK)/include/ -mlongcalls -DICACHE_FLASH -Wall -std=c99 #-Werror
LD_FLAGS = -L$(DIR_SDK)/lib --start-group -lc -lgcc -lhal -lphy -lpp -lnet80211\
	-llwip -lwpa -lcrypto -lmain -ljson -lupgrade -lssl -lpwm -lsmartconfig --end-group
LD_FLAGS += --nostdlib --gc-sections -static -T$(LINKER_SCRIPT) -Map $(OUTPUT_FILE).map

.PHONY: build clean fresh both files build_directory

build: build_directory $(OUTPUT_FILE).elf-0x00000.bin dissasembly
	@echo done!

$(OUTPUT_FILE).elf-0x00000.bin: $(OUTPUT_FILE).elf
	@echo turning $^ to binary
	@esptool --chip esp8266 elf2image --flash_size 1MB --flash_freq 40m --flash_mode dout $^ > /dev/null
#	@esptool image_info $@

build_directory:
	@mkdir -p $(DIR_BUILD)

dissasembly: $(OUTPUT_FILE).elf
	@echo dissasembling
	@$(OBJ_DUMP) -t $^ > $(DIR_BUILD)/dissasembly-sections.txt
	@$(OBJ_DUMP) -h $^ > $(DIR_BUILD)/dissasembly-headers.txt
	@$(OBJ_DUMP) -d $^ > $(DIR_BUILD)/dissasembly.txt

# extracting app_partition.o from libmain.a since from some reason ld doesnt find system_partition_table_regist() itself.
$(DIR_BUILD)/app_partition.o: $(DIR_SDK)/lib/libmain.a
	@echo extracting app_partition.o from libmain.a
	@mkdir -p $(DIR_BUILD)/temp/
	@cp $< $(DIR_BUILD)/temp/libmain.a
	@cd $(DIR_BUILD)/temp/; $(AR) x libmain.a
	@cp $(DIR_BUILD)/temp/app_partition.o $(DIR_BUILD)/app_partition.o
	@rm -f -r $(DIR_BUILD)/temp
	
$(OUTPUT_FILE).elf: $(OBJ) $(DIR_BUILD)/app_partition.o $(FILES_OBJECTS)
	@echo linking
	@$(LD) $(LD_FLAGS) -o $@ $^

$(DIR_BUILD)/init.o: $(DIR_SRC)/init.c
	@echo compiling $@
	@$(CC) $(CFLAGS) -o $@ $<

$(DIR_BUILD)/main.o: $(DIR_SRC)/main.c $(DIR_INCLUDE)/server.h $(DIR_BUILD)/files.h $(DIR_INCLUDE)/user_config.h
	@echo compiling $@
	@$(CC) $(CFLAGS) -o $@ $<

$(DIR_BUILD)/server.o: $(DIR_SRC)/server.c $(DIR_INCLUDE)/server.h $(DIR_BUILD)/files.h
	@echo compiling $@
	@$(CC) $(CFLAGS) -o $@ $<

$(FILES_OBJECTS) files $(DIR_BUILD)/files.h $(DIR_BUILD)/files.ld:
	@python3 generate_files_ld_script_and_header.py $(DIR_BUILD) $(DIR_FILES) $(OBJ_COPY) $(FILES)

clean:
	rm -f -r $(DIR_BUILD)/*
	clear

fresh: clean build

both:
	make APP_NUMBER=1
	make APP_NUMBER=2
