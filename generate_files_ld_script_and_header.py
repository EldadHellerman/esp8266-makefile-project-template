import sys
import os

def file_name_to_c_name(file_name: str) -> str:
    """
    c format is "files/index.html" -> "files_index_html".

    Args:
        file_name (str): File name (including path).

    Returns:
        str: file name as section.
    """    
    return file_name.replace(".","_").replace("/","_")

def generate_linker_script(lincker_script_file_path :str, files :list[str]):
    with open(lincker_script_file_path, "w") as script:
        script.write("/* This script was generated using python*/\r\n")
        script.write("/* File size is stored as 4 bytes after the file (plus alignment) */\r\n")
        script.write(". = ALIGN(4);\r\n")
        for file in files:
            c_name = file_name_to_c_name(file)
            script.write(f"""
symbol_file_{c_name}_start = ABSOLUTE(.);
KEEP(*(.file.{c_name}))
symbol_file_{c_name}_end = ABSOLUTE(.);
. = ALIGN(4);
file_{c_name}_size = ABSOLUTE(.);
LONG(symbol_file_{c_name}_end - symbol_file_{c_name}_start)
. = ALIGN(4);
""")

def generate_c_header(header_file_path :str, files :list[str]):
    with open(header_file_path, "w") as header:
        header.write("#ifndef FILES_H\r\n#define FILES_H\r\n")
        header.write("/* This script was generated using python*/\r\n")
        for file in files:
            c_name = file_name_to_c_name(file)
            header.write(f"""
extern int symbol_file_{c_name}_start, file_{c_name}_size;
char *file_{c_name} = (char *)(&symbol_file_{c_name}_start);
""")
        header.write("\r\n#endif //FILES_H")

def generate_obj_files(build_dir :str, files_dir :str, obj_copy_bin :str, files :list[str]):
    for file in files:
        c_name = file_name_to_c_name(file)
        os.system(f"{obj_copy_bin} -B xtensa -I binary -O elf32-xtensa-le --rename-section .data=.file.{c_name} {files_dir}/{file} {build_dir}/file_{c_name}.o")

if __name__ == "__main__":
    linker_scripts_dir = sys.argv[1] + "/files.ld"
    include_dir = sys.argv[2] + "/files.h"
    build_dir = sys.argv[3]
    files_dir = sys.argv[4]
    obj_copy_bin = sys.argv[5]
    files = sys.argv[6:]
    print("Generating files object files, c header file, and linker script")
    generate_linker_script(linker_scripts_dir , files)
    generate_c_header(include_dir, files)
    generate_obj_files(build_dir, files_dir, obj_copy_bin, files)
            


