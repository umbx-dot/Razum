#include "codegen/codegen.h"

static const char HEADER_START[] =
    "bits 64\n"
    "default rel\n"
    "org 0\n"
    "\n";

static const char HEADER_BODY[] =
    "pe@dos:\n"
    "    dw \"MZ\"                                 ; DOS signature\n"
    "    times 0x3C - ($ - pe@dos) db 0\n"
    "    dd pe@nt                                ; offset of the PE header\n"
    "pe@nt:\n"
    "    dd \"PE\"                                 ; PE signature\n"
    "    dw 0x8664                               ; machine: x86-64\n"
    "    dw 1                                    ; number of sections\n"
    "    dd 0, 0, 0                              ; timestamp, symbol table, symbol count\n"
    "    dw pe@optional_end - pe@optional        ; size of the optional header\n";

static const char HEADER_OPTIONAL[] =
    "pe@optional:\n"
    "    dw 0x020B                               ; PE32+ (64-bit)\n"
    "    db 0, 0                                 ; linker version\n"
    "    dd pe@image_end - pe@code               ; size of code\n"
    "    dd 0, 0                                 ; size of initialized and uninitialized data\n"
    "    dd rz@entry                             ; entry point\n"
    "    dd pe@code                              ; base of code\n"
    "    dq pe@base                              ; preferred image base\n"
    "    dd 4, 4                                 ; section and file alignment\n"
    "    dw 6, 0, 0, 0, 6, 0                     ; OS, image and subsystem version\n"
    "    dd 0\n"
    "    dd pe@image_end                         ; size of image\n"
    "    dd pe@code                              ; size of headers\n"
    "    dd 0                                    ; checksum\n";

static const char HEADER_END[] =
    "    dq 0x800000, 0x1000, 0x100000, 0x1000   ; stack reserve 8 MiB and commit, heap reserve and commit\n"
    "    dd 0                                    ; loader flags\n"
    "    dd 6                                    ; data directories: export, import, resource, exception, security, relocation\n"
    "    dd pe@exports, pe@exports_end - pe@exports\n"
    "    dd pe@imports, pe@imports_end - pe@imports\n"
    "    dd 0, 0, 0, 0, 0, 0\n"
    "    dd pe@relocations, pe@relocations_end - pe@relocations\n"
    "pe@optional_end:\n"
    "    db \".text\", 0, 0, 0                     ; one section holding everything after the headers\n"
    "    dd pe@image_end - pe@code               ; size in memory\n"
    "    dd pe@code                              ; address in memory\n"
    "    dd pe@image_end - pe@code               ; size in the file\n"
    "    dd pe@code                              ; offset in the file\n"
    "    dd 0, 0\n"
    "    dw 0, 0\n"
    "    dd 0xE0000060                           ; code, data, read, write, execute\n"
    "pe@code:\n";

void emit_pe_header(TextBuffer *output, bool is_dll, bool is_window_program) {
    text_append(output, HEADER_START);
    text_line(output, "pe@base equ 0x%s", is_dll ? "180000000" : "140000000");
    text_append(output, HEADER_BODY);
    text_line(output, "    dw 0x%s                               ; %s", is_dll ? "2022" : "0022",
              is_dll ? "DLL, executable, large-address aware" : "executable, large-address aware");
    text_append(output, HEADER_OPTIONAL);
    text_line(output, "    dw %d                                    ; subsystem: %s", is_window_program ? 2 : 3,
              is_window_program ? "windows (no console)" : "console");
    text_line(output, "    dw 0x%s                               ; DLL characteristics%s", is_dll ? "0060" : "0000",
              is_dll ? ": relocatable, high-entropy addresses" : "");
    text_append(output, HEADER_END);
}
