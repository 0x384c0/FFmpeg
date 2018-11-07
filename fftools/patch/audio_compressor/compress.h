/*! compress.h
 *  interface to audio compression
 *
 *  (c)2002-2007 busybee (http://beesbuzz.biz/)
 *  Licensed under the terms of the LGPL. See the file COPYING for details.
 */

#include <sys/types.h>

//! Configuration values for the compressor object
struct CompressorConfig {
	int target;
	int maxgain;
	int smooth;
};
//! Delete a compressor
static void Compressor_reset();

//! Process 16-bit signed data
// void Compressor_Process_int16(struct Compressor *obj, int16_t *audio, unsigned int count);

