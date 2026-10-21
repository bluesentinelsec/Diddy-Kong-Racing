// The game data, embedded in the executable (desktop builds, when CMake found
// assets/ at configure time: DKR_EMBED_ASSETS). The two files are pulled in
// verbatim with .incbin, so the binary carries them without a 50 MB generated
// C file; linux/reimpl.c reads these symbols before looking for assets/ on
// disk. The paths come in from CMake as the DKR_ASSETS_*_FILE macros.
//
// Symbol names get a leading underscore on Mach-O; the read-only data section
// is spelled differently on each object format.
#if defined(__APPLE__)
#define SYM(name) _##name
    .section __TEXT,__const
#elif defined(_WIN32)
#define SYM(name) name
    .section .rdata,"dr"
#else
#define SYM(name) name
    .section .rodata
#endif

    .balign 16
    .globl SYM(dkr_embedded_assets_lut)
    .globl SYM(dkr_embedded_assets_lut_end)
    .globl SYM(dkr_embedded_assets_bin)
    .globl SYM(dkr_embedded_assets_bin_end)

SYM(dkr_embedded_assets_lut):
    .incbin DKR_ASSETS_LUT_FILE
SYM(dkr_embedded_assets_lut_end):

    .balign 16
SYM(dkr_embedded_assets_bin):
    .incbin DKR_ASSETS_BIN_FILE
SYM(dkr_embedded_assets_bin_end):
