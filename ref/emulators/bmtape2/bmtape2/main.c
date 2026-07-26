/*
	bmtape2
	メイン
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <limits.h>
#include "bmtape2.h"

#define FALSE	0
#define TRUE	1
#define BUFFER_SIZE	0x100000

/*
	パス名からファイル名を得る (下請け)
*/
static char *getFileFromPath(char *file, const char *path)
{
	const char *p;

	for(p = path + strlen(path) - 1; p >= path && *p != '\\' && *p != '/'; p--)
		;
	strcpy(file, p + 1);

	return file;
}

/*
	ベーシックマスターJr.のファイル名を作成する (下請け)
*/
static char *makeFilename (char *out_file, const char *in_file, char ext)
{
	char *p;
	const char *q, *last;

	/* 拡張子を除くファイル名の末尾のポインタを求める */
	for(last = in_file + strlen(in_file) - 1; last > in_file && *last != '.'; last--)
		;
	if(last == in_file)
		last = in_file + strlen(in_file);
	
	/* ファイル名を作成する */
	memset(out_file, ' ', 8);
	for(p = out_file, q = in_file; p < out_file + 6 && q < last && *q != 0; p++, q++)
		*p = toupper(*q);
	out_file[6] = '.';
	out_file[7] = ext;

	return out_file;
}

int main(int argc, char *argv[])
{
	int i, raw_size, bin_size, text_size, address, baud = 300, mid_width = 1400, smooth_width = 3, f_zero = -1, disp_wave = FALSE;
	const char *in_path = NULL;
	char *text, out_path[PATH_MAX], out_path2[PATH_MAX], file[8], tmp[PATH_MAX];
	unsigned char *raw, *bin;

	for(i = 1; i < argc; i++) {
		if(*argv[i] != '-') {
			in_path = argv[i];
			break;
		}else if(strcmp(argv[i], "-300") == 0)
			baud = 300;
		else if(strcmp(argv[i], "-600") == 0)
			baud = 600;
		else if(strcmp(argv[i], "-1200") == 0)
			baud = 1200;
		else if(strcmp(argv[i], "-s") == 0)
			smooth_width = atoi(argv[++i]);
		else if(strcmp(argv[i], "-m") == 0)
			mid_width = atoi(argv[++i]);
		else if(strcmp(argv[i], "-t") == 0)
			f_zero = atoi(argv[++i]);
		else if(strcmp(argv[i], "-display") == 0)
			disp_wave = TRUE;
		else {
			fprintf(stderr, "Wrong option %s\n", argv[i]);
			return 1;
		}	
	}

	if(in_path == NULL) {
		fprintf(stderr, "%s [-300|-1200] [-s smoothing] [-m mid] [-t threshold] [-display] filename ...\n", getFileFromPath(tmp, argv[0]));
		return 0;
	}

	if(isWave(in_path)) {
		/* 出力ファイル名(ベタイメージ)を求める */
		getFileFromPath(out_path, in_path);
		strcat(out_path, ".bin");
		unlink(out_path);

		/* ベタイメージの領域を確保する */
		raw = malloc(BUFFER_SIZE);

		for(; i < argc; i++) {
			/* 音声をベタイメージに変換する */
			if((raw_size = wave2raw(raw, argv[i], baud, smooth_width, mid_width, f_zero, disp_wave)) < 0) {
				fprintf(stderr, "Cannot read %s", argv[i]);
				return 1;
			}
			if(appendBin(out_path, raw, raw_size) < 0) {
				fprintf(stderr, "Cannot write %s", out_path);
				return 1;
			}

			/* ベタイメージからファイルを得る */
			if(raw2s(baud, raw, raw_size) < 0)
				return 1;
		}
		return 0;
	} else if(strlen(in_path) >= 4 && memcmp(in_path + strlen(in_path) - 4, ".bin", 4) == 0) {
		/* 出力ファイル名(音声)を求める */
		getFileFromPath(out_path, in_path);
		strcat(out_path, ".wav");
		unlink(out_path);

		/* ベタイメージの領域を確保する */
		raw = malloc(BUFFER_SIZE);

		for(; i < argc; i++) {
			/* ベタイメージを音声に変換する */
			if((raw_size = readBin(argv[i], raw, BUFFER_SIZE)) < 0) {
				fprintf(stderr, "Cannot read %s\n", argv[i]);
				return 1;
			}
			if(raw2wave(out_path, baud, raw, raw_size) < 0) {
				fprintf(stderr, "Cannot write %s\n", out_path);
				return 1;
			}
		}
		return 0;
	} else {
		/* 出力ファイル名(ベタイメージ)を求める */
		getFileFromPath(out_path, in_path);
		strcat(out_path, ".bin");
		unlink(out_path);

		/* 出力ファイル名(音声)を求める */
		getFileFromPath(out_path2, in_path);
		strcat(out_path2, ".wav");
		unlink(out_path2);

		/* バイナリまたはテキストファイルの領域を確保する */
		bin = malloc(BUFFER_SIZE);
		text = (char *)bin;

		/* ベタイメージの領域を確保する */
		raw = malloc(BUFFER_SIZE);

		for(; i < argc; i++) {
			bin_size = readSRecordAbs(argv[i], bin, &address, BUFFER_SIZE, FALSE);
			if(address == 0x0070 && bin_size > 0) {
				/* 中間言語をベタイメージに変換する */
				makeFilename(file, getFileFromPath(tmp, argv[i]), (baud == 300 ? 'B': 'S'));
				raw_size = bas2raw(raw, baud, file, bin - address);
				if(appendBin(out_path, raw, raw_size) < 0) {
					fprintf(stderr, "Cannot write %s\n", out_path);
					return 1;
				}

				/* 中間言語を音声に変換する */
				if(bin2wave(out_path2, baud, file, address, bin, bin_size) < 0) {
					fprintf(stderr, "Cannot write %s\n", out_path2);
					return 1;
				}
			} else if(bin_size > 0) {
				/* バイナリファイルをベタイメージに変換する */
				makeFilename(file, getFileFromPath(tmp, argv[i]), 'B');
				raw_size = bin2raw(raw, baud, file, address, bin, bin_size);
				if(appendBin(out_path, raw, raw_size) < 0) {
					fprintf(stderr, "Cannot write %s\n", out_path);
					return 1;
				}
				
				/* バイナリファイルを音声に変換する */
				if(bin2wave(out_path2, baud, file, address, bin, bin_size) < 0) {
					fprintf(stderr, "Cannot write %s\n", out_path2);
					return 1;
				}
			} else if((text_size = readText(argv[i], text, BUFFER_SIZE)) > 0) {
				/* テキストファイルをベタイメージに変換する */
				makeFilename(file, getFileFromPath(tmp, argv[i]), 'S');
				raw_size = text2raw(raw, baud, file, text, text_size);
				if(appendText(out_path, raw, raw_size) < 0) {
					fprintf(stderr, "Cannot write %s\n", out_path);
					return 1;
				}
				
				/* テキストファイルを音声に変換する */
				if(text2wave(out_path2, baud, file, text, text_size) < 0) {
					fprintf(stderr, "Cannot write %s\n", out_path2);
					return 1;
				}
			} else {
				/* ファイルをオープンできない */
				fprintf(stderr, "Cannot read %s\n", argv[i]);
				return 1;
			}
		}
		return 0;
	}
}

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
