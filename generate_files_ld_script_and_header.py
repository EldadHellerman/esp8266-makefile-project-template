import sys, os

def file_name_to_c_name(file_name: str) -> str:
    """
    c format is "folder/index.html" -> "folder_index_html".

    Args:
        file_name (str): File name (including path).

    Returns:
        str: file name as section.
    """    
    return file_name.replace(".","_").replace("/","_")

def generate_linker_script(file_path :str, files_c_name :list[str]):
    with open(file_path, "w") as script:
        script.write("/* This script was generated using python*/\r\n")
        script.write("/* File size is stored as 4 bytes after the file (plus alignment) */\r\n")
        script.write(". = ALIGN(4);\r\n")
        for file in files_c_name:
            script.write(f"""
symbol_file_{file}_size = ABSOLUTE(.);
SHORT(symbol_file_{file}_end - symbol_file_{file}_start)
. = ALIGN(4);
symbol_file_{file}_start = ABSOLUTE(.);
KEEP(*(.file.{file}))
symbol_file_{file}_end = ABSOLUTE(.);
. = ALIGN(4);
""")

def generate_c_header(file_path :str, files_c_name :list[str]):
    with open(file_path, "w") as header:
        header.write("#ifndef FILES_H\r\n#define FILES_H\r\n")
        header.write("/* This script was generated using python*/\r\n")
        header.write("\r\n")
        header.write("#include \"c_types.h\"\r\n")
        header.write("#define file_size(FILE) (*(uint16 *)(FILE-4))\r\n")
        header.write("\r\n")
        header.write("typedef char *file_ptr;\r\n")
# extern file_ptr file_{file};
        for file in files_c_name:
            header.write(f"""
extern char symbol_file_{file}_start;
#define file_{file} ((file_ptr)&symbol_file_{file}_start)
""")
        header.write("\r\n#endif /* FILES_H */")

def generate_obj_files(build_dir :str, files_dir :str, obj_copy_bin :str, files :list[str]):
    for file in files:
        file_c_name = file_name_to_c_name(file)
        os.system(f"{obj_copy_bin} -B xtensa -I binary -O elf32-xtensa-le"
                  f" --rename-section .data=.file.{file_c_name} {files_dir}/{file} {build_dir}/file_{file_c_name}.o")

if __name__ == "__main__":
    build_dir = sys.argv[1]
    linker_scripts_dir = build_dir + "/files.ld"
    header_dir = build_dir + "/files.h"
    files_dir = sys.argv[2]
    obj_copy_bin = sys.argv[3]
    files = sys.argv[4:]
    files_c_name = [file_name_to_c_name(file) for file in files]
    print("Generating files c header, linker script, and object files")
    generate_c_header(header_dir, files_c_name)
    generate_linker_script(linker_scripts_dir , files_c_name)
    generate_obj_files(build_dir, files_dir, obj_copy_bin, files)
            


