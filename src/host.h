#ifndef DKR_HOST_H
#define DKR_HOST_H

/*
 * The services the game takes from the host (linux/). This is the whole
 * surface between the two: there is no N64 OS underneath the game any more,
 * so the thread, message-queue, scheduler and DMA plumbing is gone and what
 * remains is a handful of plain calls.
 */
#include "host.h"
#include "types.h"

// --- Logging (linux/reimpl.c) ---------------------------------------------------
void pc_log(const char *fmt, ...);
#include <stdarg.h>
void pc_vlog(const char *fmt, va_list args);

// --- Console region (linux/reimpl.c) --------------------------------------------
// The TV standard the game believes it is running on; the host picks NTSC.
#define OS_TV_TYPE_PAL 0
#define OS_TV_TYPE_NTSC 1
#define OS_TV_TYPE_MPAL 2
extern s32 osTvType;

// --- Clock ----------------------------------------------------------------------
// A 46.875 MHz counter that never returns the same value twice, like the
// N64's Count register; a trace drives it from the frame count instead.
u32 pc_clock_count(void);

// --- Game data (linux/reimpl.c) -------------------------------------------------
// The asset image: its lookup table, and copies out of it addressed the way the
// game addresses ROM (asset_loading.c turns section + offset into these).
u32 pc_asset_lut_size(void);
void pc_dmacopy(u32 romOffset, uintptr_t ramAddress, s32 numBytes);
s32 pc_asset_image_inflated(void);
void pc_asset_swap16_region(u32 romOffset, u32 numBytes);

// --- Display (linux/main.c) -----------------------------------------------------
// Hand a display list to the renderer; it is drawn and presented by the host.
void pc_gfx_task_submit(void *dlBegin, void *dlEnd);
// Pace one logic frame and return how many 60 Hz periods it covered.
s32 pc_retrace_wait(void);

// --- Audio (linux/audio.c, linux/audio_hle.c) -----------------------------------
s32 pc_ai_set_frequency(u32 frequency);
s32 pc_ai_set_next_buffer(void *buf, u32 size);
u32 pc_ai_get_length(void);
void pc_audio_hle_run(void *cmdList, s32 cmdLen, void *out, s32 frameSamples);

// --- Controllers (linux/reimpl.c) -----------------------------------------------
typedef struct PcContStatus {
    u16 type;
    u8 status;
    u8 error;
} PcContStatus;

typedef struct PcContPad {
    u16 button;
    s8 stick_x;
    s8 stick_y;
    u8 error;
} PcContPad;

// Which controllers are plugged in: a bit per port, and a status per port.
u8 pc_controllers_init(PcContStatus *status);
void pc_controllers_read(PcContPad *pads);

// --- Save data: EEPROM, controller pak and rumble (linux/reimpl.c) ----------------
s32 pc_eeprom_probe(void);
s32 pc_eeprom_read(u8 address, u8 *buffer);
s32 pc_eeprom_write(u8 address, u8 *buffer);
s32 pc_pfs_init(void *pfs, s32 channel);
s32 pc_pfs_is_plug(u8 *pattern);
s32 pc_pfs_reformat(void *pfs, s32 channel);
s32 pc_pfs_checker(void *pfs);
s32 pc_pfs_free_blocks(void *pfs, s32 *bytesNotUsed);
s32 pc_pfs_num_files(void *pfs, s32 *maxFiles, s32 *filesUsed);
s32 pc_pfs_file_state(void *pfs, s32 fileNo, void *state);
s32 pc_pfs_find_file(void *pfs, u16 companyCode, u32 gameCode, u8 *gameName, u8 *extName, s32 *fileNo);
s32 pc_pfs_allocate_file(void *pfs, u16 companyCode, u32 gameCode, u8 *gameName, u8 *extName, s32 size, s32 *fileNo);
s32 pc_pfs_delete_file(void *pfs, u16 companyCode, u32 gameCode, u8 *gameName, u8 *extName);
s32 pc_pfs_read_write_file(void *pfs, s32 fileNo, u8 flag, s32 offset, s32 size, u8 *data);
s32 pc_motor_init(void *pfs, s32 channel);
s32 pc_motor_start(void *pfs);
s32 pc_motor_stop(void *pfs);

// --- Trace harness (linux/trace.c, linux/input.c) -------------------------------
int pc_trace_autopilot(void);
int pc_trace_level_request(unsigned int frame);
int pc_trace_players(void);

#endif
