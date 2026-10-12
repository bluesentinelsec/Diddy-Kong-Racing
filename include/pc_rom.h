#ifndef PC_ROM_H
#define PC_ROM_H

/*
 * ROM layout the host must reproduce.
 *
 * The game addresses its assets by ROM offset: everything is "the asset lookup
 * table's end plus an offset", and those offsets flow through the audio DMA
 * cache, the asset loader and the bank patcher as plain numbers (the N64 has no
 * reason to treat them as anything else). The host serves the same bytes from
 * assets/assets.bin, but it must keep the numbers identical to the N64's, since
 * things like which DMA buffer a sample lands in depend on them - so the linker
 * symbols are replaced by the ROM offsets from ver/splat (segments assets.lut
 * and assets).
 */

#if defined(VERSION_US_V80)
#define PC_ASSETS_LUT_ROM_START 0xED0E0u
#define PC_ASSETS_LUT_ROM_END 0xED1B0u
#elif defined(VERSION_US_V77)
#define PC_ASSETS_LUT_ROM_START 0xECB60u
#define PC_ASSETS_LUT_ROM_END 0xECC30u
#else
#error "pc_rom.h: no asset table ROM offsets for this version"
#endif

#define __ASSETS_LUT_START ((u8 *) (uintptr_t) PC_ASSETS_LUT_ROM_START)
#define __ASSETS_LUT_END ((u8 *) (uintptr_t) PC_ASSETS_LUT_ROM_END)

#endif
