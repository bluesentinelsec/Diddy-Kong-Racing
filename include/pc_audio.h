#ifndef PC_AUDIO_H
#define PC_AUDIO_H

/**
 * @file pc_audio.h
 * Mix-rate scaling between the game's synthesizer and the host (linux/audio.c).
 *
 * The synthesizer is authored for OUTPUT_RATE (22050 Hz): a voice pitch of 1.0
 * plays its sample at that rate, and the reverb's delay lines are counted in
 * those samples. The host may mix at a higher rate (--audio-rate); these two
 * factors carry that through, so everything still sounds at the right pitch and
 * the reverb keeps its timing. Both are 1.0 when the host mixes at OUTPUT_RATE.
 */
extern float gPcPitchScale;     // OUTPUT_RATE / mix rate: multiplied into every voice pitch
extern float gPcAudioRateScale; // mix rate / OUTPUT_RATE: multiplied into the reverb's sample counts

#endif
