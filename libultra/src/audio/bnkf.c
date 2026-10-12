/*====================================================================
 * bnkf.c
 *
 * Copyright 1993, Silicon Graphics, Inc.
 * All Rights Reserved.
 *
 * This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics,
 * Inc.; the contents of this file may not be disclosed to third
 * parties, copied or duplicated in any form, in whole or in part,
 * without the prior written permission of Silicon Graphics, Inc.
 *
 * RESTRICTED RIGHTS LEGEND:
 * Use, duplication or disclosure by the Government is subject to
 * restrictions as set forth in subdivision (c)(1)(ii) of the Rights
 * in Technical Data and Computer Software clause at DFARS
 * 252.227-7013, and/or in similar or successor clauses in the FAR,
 * DOD or NASA FAR Supplement. Unpublished - rights reserved under the
 * Copyright Laws of the United States.
 *====================================================================*/

#include <libaudio.h>
#include <os_internal.h>
#include <ultraerror.h>

/*
 * ### when the file format settles down a little, I'll remove these
 * ### for efficiency.
 */
static  void _bnkfPatchBank(ALBank *bank, s32 offset, s32 table);
static  void _bnkfPatchInst(ALInstrument *i, s32 offset, s32 table);
static  void _bnkfPatchSound(ALSound *s, s32 offset, s32 table);
static  void _bnkfPatchWaveTable(ALWaveTable *w, s32 offset, s32 table);

#ifdef TARGET_PC
/*
 * PC: the bank file is a big-endian image whose "pointers" are u32 file
 * offsets, and every struct in it is wider on a 64-bit host. Rather than patch
 * the image in place, walk it and build the ALBankFile -> ALBank -> ALInstrument
 * -> ALSound -> ALWaveTable/ALEnvelope/ALKeyMap/ALADPCMBook/ALADPCMloop tree out
 * of host-allocated nodes. Nodes shared by several sounds are built once and
 * shared again, keyed by file offset. The caller's buffer ends up holding just
 * the host ALBankFile header, written last, after everything was read from it.
 */
#include <stddef.h>
#include <string.h>
#include "pc_asset.h"

extern void isv_printf(const char *fmt, ...);
/* RAW16 sample data is big-endian s16 and is DMA'd straight out of the asset
 * image on demand by __amDMA, so it is swapped once at the source. (ADPCM waves
 * are a byte stream and must NOT come through this.) linux/reimpl.c */
extern void pc_asset_swap16_region(u32 romOffset, u32 numBytes);

#define BNKF_MAX_NODES 8192
typedef struct {
    u32 off;
    void *node;
} BnkfNode;
static BnkfNode sBnkfNodes[BNKF_MAX_NODES];
static s32 sBnkfNodeCount;

static void *_bnkfFind(u32 off)
{
    s32 i;
    for (i = 0; i < sBnkfNodeCount; i++) {
        if (sBnkfNodes[i].off == off)
            return sBnkfNodes[i].node;
    }
    return NULL;
}

static void _bnkfRemember(u32 off, void *node)
{
    if (sBnkfNodeCount == BNKF_MAX_NODES) {
        isv_printf("*** bnkf: node registry overflow (>%d); shared nodes will be duplicated ***\n", BNKF_MAX_NODES);
        return;
    }
    sBnkfNodes[sBnkfNodeCount].off = off;
    sBnkfNodes[sBnkfNodeCount].node = node;
    sBnkfNodeCount++;
}

static ALEnvelope *_bnkfEnvelope(const u8 *img, u32 off)
{
    ALEnvelope *e = _bnkfFind(off);
    if (e != NULL)
        return e;
    e = pc_alloc(sizeof(ALEnvelope));
    e->attackTime = pc_be32(img, off + 0);
    e->decayTime = pc_be32(img, off + 4);
    e->releaseTime = pc_be32(img, off + 8);
    e->attackVolume = img[off + 12];
    e->decayVolume = img[off + 13];
    _bnkfRemember(off, e);
    return e;
}

static ALKeyMap *_bnkfKeyMap(const u8 *img, u32 off)
{
    ALKeyMap *k = _bnkfFind(off);
    if (k != NULL)
        return k;
    k = pc_alloc(sizeof(ALKeyMap));
    k->velocityMin = img[off + 0];
    k->velocityMax = img[off + 1];
    k->keyMin = img[off + 2];
    k->keyMax = img[off + 3];
    k->keyBase = img[off + 4];
    k->detune = (s8) img[off + 5];
    _bnkfRemember(off, k);
    return k;
}

static ALADPCMBook *_bnkfBook(const u8 *img, u32 off)
{
    ALADPCMBook *b = _bnkfFind(off);
    s32 i, entries;
    if (b != NULL)
        return b;
    entries = (s32) pc_be32(img, off + 4) * (s32) pc_be32(img, off + 0) * 8;
    b = pc_alloc(offsetof(ALADPCMBook, book) + (entries > 0 ? entries : 1) * sizeof(s16));
    b->order = (s32) pc_be32(img, off + 0);
    b->npredictors = (s32) pc_be32(img, off + 4);
    for (i = 0; i < entries; i++) {
        b->book[i] = (s16) pc_be16(img, off + 8 + i * 2);
    }
    _bnkfRemember(off, b);
    return b;
}

static ALADPCMloop *_bnkfADPCMLoop(const u8 *img, u32 off)
{
    ALADPCMloop *l = _bnkfFind(off);
    s32 i;
    if (l != NULL)
        return l;
    l = pc_alloc(sizeof(ALADPCMloop));
    l->start = pc_be32(img, off + 0);
    l->end = pc_be32(img, off + 4);
    l->count = pc_be32(img, off + 8);
    for (i = 0; i < 16; i++) { /* ADPCM_STATE: 16 BE s16 predictor values */
        l->state[i] = (s16) pc_be16(img, off + 12 + i * 2);
    }
    _bnkfRemember(off, l);
    return l;
}

static ALRawLoop *_bnkfRawLoop(const u8 *img, u32 off)
{
    ALRawLoop *l = _bnkfFind(off);
    if (l != NULL)
        return l;
    l = pc_alloc(sizeof(ALRawLoop));
    l->start = pc_be32(img, off + 0);
    l->end = pc_be32(img, off + 4);
    l->count = pc_be32(img, off + 8);
    _bnkfRemember(off, l);
    return l;
}

/* Disk ALWaveTable: base(0) len(4) type(8) flags(9) waveInfo(12): adpcm {loop(12), book(16)} / raw {loop(12)} */
static ALWaveTable *_bnkfWaveTable(const u8 *img, u32 off, uintptr_t table)
{
    ALWaveTable *w = _bnkfFind(off);
    u32 sub;
    if (w != NULL)
        return w;
    w = pc_alloc(sizeof(ALWaveTable));
    w->base = (u8 *) (pc_be32(img, off + 0) + table); /* ROM address of the sample data */
    w->len = (s32) pc_be32(img, off + 4);
    w->type = img[off + 8];
    w->flags = 1; /* patched */
    if (w->type == AL_ADPCM_WAVE) {
        sub = pc_be32(img, off + 12);
        w->waveInfo.adpcmWave.loop = sub ? _bnkfADPCMLoop(img, sub) : NULL;
        w->waveInfo.adpcmWave.book = _bnkfBook(img, pc_be32(img, off + 16));
    } else if (w->type == AL_RAW16_WAVE) {
        sub = pc_be32(img, off + 12);
        w->waveInfo.rawWave.loop = sub ? _bnkfRawLoop(img, sub) : NULL;
        pc_asset_swap16_region((u32) (uintptr_t) w->base, (u32) w->len);
    }
    _bnkfRemember(off, w);
    return w;
}

/* Disk ALSound: envelope(0) keyMap(4) wavetable(8) samplePan(12) sampleVolume(13) flags(14) */
static ALSound *_bnkfSound(const u8 *img, u32 off, uintptr_t table)
{
    ALSound *s = _bnkfFind(off);
    if (s != NULL)
        return s;
    s = pc_alloc(sizeof(ALSound));
    s->envelope = _bnkfEnvelope(img, pc_be32(img, off + 0));
    s->keyMap = _bnkfKeyMap(img, pc_be32(img, off + 4));
    s->wavetable = _bnkfWaveTable(img, pc_be32(img, off + 8), table);
    s->samplePan = img[off + 12];
    s->sampleVolume = img[off + 13];
    s->flags = 1;
    _bnkfRemember(off, s);
    return s;
}

/* Disk ALInstrument: 12 x u8 (volume..vibDelay), bendRange(12), soundCount(14), soundArray(16, u32 each) */
static ALInstrument *_bnkfInst(const u8 *img, u32 off, uintptr_t table)
{
    ALInstrument *inst = _bnkfFind(off);
    s32 i, count;
    if (inst != NULL)
        return inst;
    count = (s16) pc_be16(img, off + 14);
    inst = pc_alloc(offsetof(ALInstrument, soundArray) + (count > 0 ? count : 1) * sizeof(ALSound *));
    inst->volume = img[off + 0];
    inst->pan = img[off + 1];
    inst->priority = img[off + 2];
    inst->flags = 1;
    inst->tremType = img[off + 4];
    inst->tremRate = img[off + 5];
    inst->tremDepth = img[off + 6];
    inst->tremDelay = img[off + 7];
    inst->vibType = img[off + 8];
    inst->vibRate = img[off + 9];
    inst->vibDepth = img[off + 10];
    inst->vibDelay = img[off + 11];
    inst->bendRange = (s16) pc_be16(img, off + 12);
    inst->soundCount = (s16) count;
    for (i = 0; i < count; i++) {
        inst->soundArray[i] = _bnkfSound(img, pc_be32(img, off + 16 + i * 4), table);
    }
    _bnkfRemember(off, inst);
    return inst;
}

/* Disk ALBank: instCount(0) flags(2) pad(3) sampleRate(4) percussion(8) instArray(12, u32 each) */
static ALBank *_bnkfBank(const u8 *img, u32 off, uintptr_t table)
{
    ALBank *bank = _bnkfFind(off);
    s32 i, count;
    u32 perc;
    if (bank != NULL)
        return bank;
    count = (s16) pc_be16(img, off + 0);
    bank = pc_alloc(offsetof(ALBank, instArray) + (count > 0 ? count : 1) * sizeof(ALInstrument *));
    bank->instCount = (s16) count;
    bank->flags = 1;
    bank->sampleRate = (s32) pc_be32(img, off + 4);
    perc = pc_be32(img, off + 8);
    bank->percussion = perc ? _bnkfInst(img, perc, table) : NULL;
    for (i = 0; i < count; i++) {
        u32 io = pc_be32(img, off + 12 + i * 4);
        bank->instArray[i] = io ? _bnkfInst(img, io, table) : NULL;
    }
    _bnkfRemember(off, bank);
    return bank;
}

void alSeqFileNew(ALSeqFile *file, u8 *base)
{
    const u8 *img = (const u8 *) file;
    s32 count = (s16) pc_be16(img, 2);
    s32 i;
    /* The image header is 4 + 8n bytes; the host header is wider and overlaps it. */
    u8 *tmp = pc_alloc(4 + 8 * (count > 0 ? count : 0));

    memcpy(tmp, img, 4 + 8 * (count > 0 ? count : 0));
    file->revision = (s16) pc_be16(tmp, 0);
    file->seqCount = (s16) count;
    for (i = 0; i < count; i++) {
        file->seqArray[i].offset = base + pc_be32(tmp, 4 + i * 8);
        file->seqArray[i].len = (s32) pc_be32(tmp, 8 + i * 8);
    }
    pc_free(tmp);
}

void alBnkfNew(ALBankFile *file, u8 *table)
{
    const u8 *img = (const u8 *) file;
    s32 count = (s16) pc_be16(img, 2);
    s16 revision = (s16) pc_be16(img, 0);
    ALBank **banks;
    s32 i;

    sBnkfNodeCount = 0; /* fresh image, fresh registry */
    ALFailIf(revision != AL_BANK_VERSION, ERR_ALBNKFNEW);

    banks = pc_alloc((count > 0 ? count : 1) * sizeof(ALBank *));
    for (i = 0; i < count; i++) {
        u32 bo = pc_be32(img, 4 + i * 4);
        banks[i] = bo ? _bnkfBank(img, bo, (uintptr_t) table) : NULL;
    }
    /* Everything has been read; now the host header may overwrite the image. */
    file->revision = revision;
    file->bankCount = (s16) count;
    for (i = 0; i < count; i++) {
        file->bankArray[i] = banks[i];
    }
    pc_free(banks);
}

#else /* TARGET_PC */

void alSeqFileNew(ALSeqFile *file, u8 *base)
{
    s32 offset = (s32) base;
    s32 i;


    /*
     * patch the file so that offsets are pointers
     */
    for (i = 0; i < file->seqCount; i++) {
        file->seqArray[i].offset = (u8 *)((u8 *)file->seqArray[i].offset + offset);
    }
}

void alBnkfNew(ALBankFile *file, u8 *table)
{
    s32 offset = (s32) file;
    s32 woffset = (s32) table;

    s32 i;


    /*
     * check the file format revision in debug libraries
     */
    ALFailIf(file->revision != AL_BANK_VERSION, ERR_ALBNKFNEW);

    /*
     * patch the file so that offsets are pointers
     */
    for (i = 0; i < file->bankCount; i++) {
        file->bankArray[i] = (ALBank *)((u8 *)file->bankArray[i] + offset);
        if(file->bankArray[i])
            _bnkfPatchBank(file->bankArray[i], offset, woffset);
    }
}

void _bnkfPatchBank(ALBank *bank, s32 offset, s32 table)
{
    s32 i;

    if (bank->flags)
        return;

    bank->flags = 1;


    if (bank->percussion) {
        bank->percussion = (ALInstrument *)((u8 *)bank->percussion + offset);
        _bnkfPatchInst(bank->percussion, offset, table);
    }
    
    for (i = 0; i < bank->instCount; i++) {
        bank->instArray[i] = (ALInstrument *)((u8 *)bank->instArray[i] +
                                              offset);
        if(bank->instArray[i])
            _bnkfPatchInst(bank->instArray[i], offset, table);
    }
}

void _bnkfPatchInst(ALInstrument *inst, s32 offset, s32 table)
{
    s32 i;

    if (inst->flags)
        return;

    inst->flags = 1;


    for (i = 0; i < inst->soundCount; i++) {
        inst->soundArray[i] = (ALSound *)((u8 *)inst->soundArray[i] +
                                          offset);
        _bnkfPatchSound(inst->soundArray[i], offset, table);

    }
}

void _bnkfPatchSound(ALSound *s, s32 offset, s32 table)
{
    if (s->flags)
        return;

    s->flags = 1;


    s->envelope  = (ALEnvelope *)((u8 *)s->envelope + offset);
    s->keyMap    = (ALKeyMap *)((u8 *)s->keyMap + offset);


    s->wavetable = (ALWaveTable *)((u8 *)s->wavetable + offset);
    _bnkfPatchWaveTable(s->wavetable, offset, table);
}

void _bnkfPatchWaveTable(ALWaveTable *w, s32 offset, s32 table)
{
    if (w->flags)
        return;

    w->flags = 1;


    w->base += table;

    /* sct 2/14/96 - patch wavetable loop info based on type. */
    if (w->type == AL_ADPCM_WAVE)
    {
	w->waveInfo.adpcmWave.book  = (ALADPCMBook *)((u8 *)w->waveInfo.adpcmWave.book + offset);
	if (w->waveInfo.adpcmWave.loop)
	    w->waveInfo.adpcmWave.loop = (ALADPCMloop *)((u8 *)w->waveInfo.adpcmWave.loop + offset);

    }
    else if (w->type == AL_RAW16_WAVE)
    {
	if (w->waveInfo.rawWave.loop)
	    w->waveInfo.rawWave.loop = (ALRawLoop *)((u8 *)w->waveInfo.rawWave.loop + offset);

    }
}

#endif /* TARGET_PC */
