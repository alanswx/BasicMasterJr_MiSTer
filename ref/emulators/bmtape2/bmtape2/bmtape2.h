/*
	bmtape2
	ƒwƒbƒ_
*/

#ifndef KANSAS_H
#define KANSAS_H

#include "srecord.h"

#define MIN(x, y)	((x) < (y) ? (x): (y))
#define MAX(x, y)	((x) > (y) ? (x): (y))

/* io.c */
int isWave(const char *);
void *readWave(int *, int *, const char *);
int writeWave(const char *, unsigned char *, int);
int appendWave(const char *, unsigned char *, int);
int readBin(const char *, void *, int);
int writeBin(const char *, const void *, int);
int appendBin(const char *, const void *, int);
int readText(const char *, void *, int);
int writeText(const char *, const void *, int);
int appendText(const char *, const void *, int);

/* kansas.c */
int wave2raw(unsigned char *, const char *, int, int, int, int, int);
int putWaveByte(unsigned char *, int, unsigned char);
int putWaveBytes(unsigned char *, int, const unsigned char *, int);
int putWaveSilent(unsigned char *, int);
int getWaveSize(int, int);
int getWaveSizeSec(int);
int raw2wave(const char *, int, const unsigned char *, int);

/* bm.c */
int raw2s(int, const unsigned char *, int);
int bin2raw(unsigned char *, int, const char *, int, const unsigned char *, int);
int bas2raw(unsigned char *, int, const char *, const unsigned char *);
int text2raw(unsigned char *, int, const char *, const char *, int);
int bin2wave(const char *, int, const char *, int, const unsigned char *, int);
int text2wave(const char *, int, const char *, const char *, int);

#endif

/*
	Copyright (c) 2010 maruhiro
	All rights reserved. 

	Redistribution and use in source and binary forms, 
	with or without modification, are permitted provided that 
	the following conditions are met: 

	 1. Redistributions of source code must retain the above copyright notice, 
	    this list of conditions and the following disclaimer. 

	 2. Redistributions in binary form must reproduce the above copyright notice, 
	    this list of conditions and the following disclaimer in the documentation 
	    and/or other materials provided with the distribution. 

	THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, 
	INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND 
	FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL 
	THE REGENTS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
	SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
	PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; 
	OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
	WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR 
	OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF 
	ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

/* eof */
