/*
	bmtape2
	BASIC MASTER Jr.のファイルを読み書きする
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "bmtape2.h"

#define FALSE	0
#define TRUE	1

/*
	ファイル名をチェックする (raw2s_300, raw2s_1200の下請け)
*/
static int chkFilename(const unsigned char *file)
{
	const unsigned char *p;

	for(p = file; p < file + 8; p++)
		if(*p < 0x20)
			return FALSE;
	return TRUE;
}

/*
	ファイル名を得る (raw2s_300, raw2s_1200の下請け)
*/
static char *getFilename(char *out, const unsigned char *in)
{
	const unsigned char *p;
	char *q = out;

	for(p = in; p < in + 8; p++)
		if(*p != ' ') {
			if(
			*p < 0x20 ||
			*p == '\\' ||
			*p == '/' ||
			*p == ':' ||
			*p == '*' ||
			*p == '?' ||
			*p == '"' ||
			*p == '<' ||
			*p == '>' ||
			*p == '|'
			)
				*q++ = '_';
			else
				*q++ = (char )*p;
		}
	*q = 0;
	return out;
}

/*
	改行コードを変換する (raw2s_300の下請け)
*/
static char *decodeReturn(char *buf, int size)
{
	char *p;

	for(p = buf; p < buf + size; p++)
		if(*p == 0x0d)
			*p = '\n';
	return buf;
}

/*
	300baudのベタイメージからファイルを得る (raw2sの下請け)
*/
static int raw2s_300(const unsigned char *raw, int raw_size)
{
	int i, kind, asc_block, bin_block, bsize, hsum, dsum, address, result;
	const unsigned char *p = raw;
	unsigned char file[8 + 1], prev_file[8 + 1] = "", data[256];
	char path[PATH_MAX];

	while(p < raw + raw_size) {
		/* 先頭の0xff */
		if(*p == 0xff)
			while(*++p == 0xff)
				;

		/* 開始か? */
		if(*p++ != 0x01)
			continue;
		
		/* 最終ブロックか? */
		hsum = -*p;
		kind = *p++;

		/* ASCIIのブロック番号を得る */
		hsum -= *p;
		asc_block = *p++;

		/* ファイル名を得る */
		hsum -= *p;
		file[0] = *p++;
		hsum -= *p;
		file[1] = *p++;
		hsum -= *p;
		file[2] = *p++;
		hsum -= *p;
		file[3] = *p++;
		hsum -= *p;
		file[4] = *p++;
		hsum -= *p;
		file[5] = *p++;
		hsum -= *p;
		file[6] = *p++;
		hsum -= *p;
		file[7] = *p++;
		file[8] = 0;

		/* バイナリのブロック番号を得る */
		hsum -= *p;
		bin_block = *p++;

		/* サイズを得る */
		hsum -= *p;
		bsize = *p++;
		if(bsize == 0)
			bsize = 0x100;
	
		/* アドレスを得る */
		hsum -= *p;
		address = (int )*p++ << 8;
		hsum -= *p;
		address = address | (int )*p++;

		/* ヘッダのサムを得る */
		if(*p != (hsum & 0xff))
			fprintf(stderr, "HEADER SUM ERROR AT %s.%d %04x-%04x (H:%02x A:%02x)\n", file, bin_block, address, address + bsize - 1, *p, (hsum & 0xff));
		p++;
	
		/* データを得る */
		dsum = 0;
		for(i = 0; i < bsize; i++) {
			dsum -= *p;
			data[i] = *p++;
		}

		/* データのサムを得る */
		if(*p != (dsum & 0xff))
			fprintf(stderr, "DATA SUM ERROR AT %s.%d %04x-%04x (H:%02x A:%02x)\n", file, bin_block, address, address + bsize - 1, *p, (dsum & 0xff));
		p++;

		/* 終了 */
		p++;

		switch(kind) {
		case 0x00:
			break;
		case 0x01:
		case 0x10:
			if(!chkFilename(file))
				continue;
			getFilename(path, file);
			if(kind == 0x01) {
				if(strcmp(file, prev_file) != 0)
					result = writeSRecordAbs(path, data, address, bsize);
				else
					result = appendSRecordAbs(path, data, address, bsize);
				if(result < 0) {
					fprintf(stderr, "Cannot write %s\n", path);
					return -1;
				}
				fprintf(stderr, "%s.%d\t%04x %d\n", file, bin_block, address, bsize);
			} else if(kind == 0x10 && bsize >= 2 && memcmp(data, "\xff\xff", 2) != 0) {
				decodeReturn(data, bsize);
				if(strcmp(file, prev_file) != 0)
					writeText(path, data, bsize);
				else
					appendText(path, data, bsize);
				fprintf(stderr, "%s.%d\t%04x %d\n", file, asc_block, address, bsize);
			}
			strcpy(prev_file, file);
			break;
		}
	}

	return (int )(p - raw);
}

/*
	1200baudのベタイメージからファイルを得る (raw2sの下請け)
*/
static int raw2s_1200(const unsigned char *raw, int raw_size)
{
	int begin1, end1, size1, begin2, end2, size2;
	unsigned char file[8 + 1];
	const unsigned char *p = raw;
	char path[PATH_MAX];
	unsigned char buf[16] = {
		0x50, 0x0b,
		0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00,
		0x00, 0x00,
		0xaf, 0xff,
		0x00, 0x00,
		0x04, 0x55
	};

	while(p < raw + raw_size) {
		/* 先頭の0x55 */
		if(*p == 0x55)
			while(*++p == 0x55)
				;
		
		/* 開始か? */
		if(*p++ != 0xaa)
			continue;
	
		/* データ部 先頭アドレス */
		begin1 = (int )(*p++) << 8;
		begin1 = begin1 | (int )(*p++);

		/* データ部 終了アドレス */
		end1 = (int )(*p++) << 8;
		end1 = end1 | (int )(*p++);
		size1 = end1 - begin1 + 1;

		/* 変数部 先頭アドレス */
		begin2 = (int )(*p++) << 8;
		begin2 = begin2 | (int )(*p++);
		if(begin2 != 0)
			begin2++;

		/* 変数部 終了アドレス */
		end2 = (int )(*p++) << 8;
		end2 = end2 | (int )(*p++);
		if(end2 != 0)
			size2 = end2 - begin2 + 1;
		else
			size2 = 0;

		/* ファイル名 */
		file[0] = *p++;
		file[1] = *p++;
		file[2] = *p++;
		file[3] = *p++;
		file[4] = *p++;
		file[5] = *p++;
		file[6] = *p++;
		file[7] = *p++;
		file[8] = 0;
		if(!chkFilename(file))
			continue;
		fprintf(stderr, "%s\t%04x %d %04x %d\n", file, begin1, size1, begin2, size2);
		fflush(stderr);
		getFilename(path, file);

		if(size1 < 0 || size2 < 0)
			continue;

		/* 変数部 */
		unlink(path);
		if(0 < size2 && size2 <= raw_size - (int )(p - raw) - size1)
			appendSRecordAbs(path, p + size1, begin2, size2);

		/* データ部 */
		if(0 < size1 && size1 <= raw_size - (int )(p - raw))
			appendSRecordAbs(path, p, begin1, size1);

		/* アドレス */
		if(size2 > 0) {
			buf[2] = begin2 >> 8;
			buf[3] = begin2 & 0xff;
			buf[4] = begin1 >> 8;
			buf[5] = begin1 & 0xff;
			buf[6] = end2 >> 8;
			buf[7] = end2 & 0xff;
			buf[8] = buf[12] = (end1 - 1) >> 8;
			buf[9] = buf[13] = (end1 - 1) & 0xff;
			appendSRecordAbs(path, buf, 0x0070, 0x10);
		}

		p += size1 + size2;
	}

	return (int )(p - raw);
}

/*
	ベタイメージからファイルを得る
*/
int raw2s(int baud, const unsigned char *raw, int raw_size)
{
	if(baud == 300)
		return raw2s_300(raw, raw_size);
	else if(baud == 1200)
		return raw2s_1200(raw, raw_size);
	else
		return -1;
}

/*
	1ブロック分バイナリを300baudのベタイメージに変換する (下請け)
*/
static int bin2raw_300_block(unsigned char *raw, const char *file, int kind, int asc_block, int bin_block, int address, const unsigned char *bin, int bin_size)
{
	int i, bsize, hsum, dsum = 0;
	unsigned char *p = raw;

	if(bin_size < 0)
		bsize = 0;
	else if(bin_size >= 0x100)
		bsize = 0x100;
	else
		bsize = bin_size;

	/* 先頭の0xff */
	for(i = 0; i < 65; i++)
		*p++ = 0xff;

	/* 開始 */
	*p++ = 0x01;

	/* ブロックの種類 */
	if(bin_size <= 0)
		*p = 0x00;
	else
		*p = kind;
	hsum = -*p++;

	/* ASCIIのブロック番号 */
	*p = asc_block;
	hsum -= *p++;

	/* ファイル名 */
	*p = file[0];
	hsum -= *p++;
	*p = file[1];
	hsum -= *p++;
	*p = file[2];
	hsum -= *p++;
	*p = file[3];
	hsum -= *p++;
	*p = file[4];
	hsum -= *p++;
	*p = file[5];
	hsum -= *p++;
	*p = file[6];
	hsum -= *p++;
	*p = file[7];
	hsum -= *p++;

	/* バイナリのブロック番号 */
	*p = bin_block;
	hsum -= *p++;

	/* ブロックのサイズ */
	*p = bsize & 0xff;
	hsum -= *p++;

	/* アドレス */
	*p = address >> 8;
	hsum -= *p++;
	*p = address & 0xff;
	hsum -= *p++;

	/* ヘッダのサム */
	*p++ = hsum & 0xff;

	/* データ */
	for(i = 0; i < bsize; i++) {
		*p = *bin++;
		dsum -= *p++;
	}

	/* データのサム */
	*p++ = dsum & 0xff;

	return (int )(p - raw);
}

/*
	1ブロック分バイナリを300baudのベタイメージに変換し, 次のブロックへ移動する (下請け)
*/
static int bin2raw_300_block_next(unsigned char **raw, const char *file, int kind, int *asc_block, int *bin_block, int *address, const unsigned char **bin, int *bin_size)
{
	*raw += bin2raw_300_block(*raw, file, kind, *asc_block, *bin_block, *address, *bin, *bin_size);
	if(*bin_size <= 0)
		return FALSE;
	if(*bin_size <= 0x100)
		*bin_size = 0;
	else {
		*bin += 0x100;
		*bin_size -= 0x100;
		*address += 0x100;
		if(kind == 0x01)
			(*bin_block)++;
		else if(kind == 0x10)
			(*asc_block)++;
	}
	return TRUE;
}

/*
	バイナリを300baudのベタイメージに変換する (bin2rawの下請け)
*/
static int bin2raw_300(unsigned char *raw, const char *file, int address, const unsigned char *bin, int bin_size)
{
	int asc_block = 1, bin_block = 1;
	unsigned char *p = raw;

	while(bin2raw_300_block_next(&p, file, 0x01, &asc_block, &bin_block, &address, &bin, &bin_size))
		;
	return (int )(p - raw);
}

/*
	バイナリを1200baudのベタイメージに変換する (bin2rawの下請け)
*/
static int bin2raw_1200(unsigned char *raw, const char *file, int begin1, int end1, const unsigned char *bin1, int begin2, int end2, const unsigned char *bin2)
{
	int i;
	unsigned char *p = raw;

	/* 先頭の0x55 */
	for(i = 0; i < 255; i++)
		*p++ = 0x55;

	/* 開始 */
	*p++ = 0xaa;

	/* データ部 開始アドレス */
	*p++ = begin1 >> 8;
	*p++ = begin1 & 0xff;

	/* データ部 終了アドレス */
	*p++ = end1 >> 8;
	*p++ = end1 & 0xff;

	if(bin2 != NULL) {
		/* 変数部 先頭アドレス */
		*p++ = (begin2 - 1) >> 8;
		*p++ = (begin2 - 1) & 0xff;

		/* 変数部 終了アドレス */
		*p++ = end2 >> 8;
		*p++ = end2 & 0xff;
	} else {
		/* 変数部 先頭アドレス */
		*p++ = 0;
		*p++ = 0;

		/* 変数部 終了アドレス */
		*p++ = 0;
		*p++ = 0;
	}

	/* ファイル名 */
	*p++ = file[0];
	*p++ = file[1];
	*p++ = file[2];
	*p++ = file[3];
	*p++ = file[4];
	*p++ = file[5];
	*p++ = file[6];
	*p++ = file[7];

	/* データ部 */
	for(i = begin1; i <= end1; i++)
		*p++ = *bin1++;

	/* 変数部 */
	if(bin2 != NULL)
		for(i = begin2; i <= end2; i++)
			*p++ = *bin2++;

	/* 終端? */
	*p++ = 0xf0;

	return (int )(p - raw);
}

/*
	バイナリを1200baudのベタイメージに変換する
*/
int bin2raw(unsigned char *raw, int baud, const char *file, int address, const unsigned char *bin, int bin_size)
{
	if(baud == 300)
		return bin2raw_300(raw, file, address, bin, bin_size);
	else if(baud == 1200)
		return bin2raw_1200(raw, file, address, address + bin_size - 1, bin, 0, 0, NULL);
	else
		return -1;
}

/*
	中間言語を300baudのベタイメージに変換する (bas2rawの下請け)
*/
static int bas2raw_300(unsigned char *raw, const char *file, const unsigned char *bin0, int begin1, int end1, const unsigned char *bin1, int begin2, int end2, const unsigned char *bin2)
{
	int address, size, block = 1;
	unsigned char *p = raw;

	/* 変数部 */
	for(address = begin2, size = end2 - begin2 + 1; address <= end2; address += 0x100, bin2 += 0x100)
		p += bin2raw_300_block(p, file, 0x01, 0x39, block++, address, bin2, size);

	/* データ部 */
	for(address = begin1, size = end1 - begin1 + 1; address <= end1; address += 0x100, bin1 += 0x100)
		p += bin2raw_300_block(p, file, 0x01, 0x39, block++, address, bin1, size);

	/* アドレス */
	p += bin2raw_300_block(p, file, 0x01, 0x39, 1, 0x0070, bin0, 0x10);

	/* 終了 */
	p += bin2raw_300_block(p, file, 0x00, 0x39, 1, 0x0070, bin0, 1);
	return (int )(p - raw);
}

/*
	中間言語をベタイメージに変換する
*/
int bas2raw(unsigned char *raw, int baud, const char *file, const unsigned char *bin)
{
	int begin1 = (((int )bin[0x74] << 8) | bin[0x75]);
	int end1   = (((int )bin[0x78] << 8) | bin[0x79]) + 1;
	int begin2 = (((int )bin[0x72] << 8) | bin[0x73]);
	int end2   = (((int )bin[0x76] << 8) | bin[0x77]);

	if(baud == 300)
		return bas2raw_300(raw, file, bin + 0x70, begin1, end1, bin + begin1, begin2, end2, bin + begin2);
	else if(baud == 1200)
		return bin2raw_1200(raw, file, begin1, end1, bin + begin1, begin2, end2, bin + begin2);
	else
		return -1;
}

/*
	1行の長さを得る(改行を含む) (text2binの下請け)
*/
static int textlen(const char *buf)
{
	const char *p;

	for(p = buf; *p != '\n' && *p != '\r' && *p != 0; p++)
		;
	return (int )(p - buf) + 1;
}

/*
	テキストを1ブロック分書き込み, 改行を変換する (test2raw_300, text2wav_300の下請け)
*/
static int text2bin(unsigned char *bin, const char **text, int *text_size)
{
	int bin_size = 0, len;

	while(*text_size > 0 && bin_size + textlen(*text) < 0x100) {
		len = textlen(*text);
		memcpy(bin, *text, len); bin[len - 1] = 0x0d;

		bin += len;
		bin_size += len;

		*text += len;
		*text_size -= len;
		if(len >= 1 && memcmp(*text - 1, "\r\n", 2) == 0) {
			(*text)++;
			(*text_size)--;
		}
	}
	return bin_size;
}

/*
	テキストを300baudのベタイメージに変換する (text2rawの下請け)
*/
static int text2raw_300(unsigned char *raw, const char *file, const char *text, int text_size)
{
	int asc_block = 1, bin_size;
	unsigned char bin[0x100], *p = raw;

	while(text_size > 0) {
		bin_size = text2bin(bin, &text, &text_size);
		p += bin2raw_300_block(p, file, 0x10, asc_block, 1, 0x600, bin, bin_size);
		p += bin2raw_300_block(p, file, 0x00, asc_block, 1, 0x600, bin, 1);
		asc_block++;
	}

	bin[1] = bin[0] = 0xff;
	p += bin2raw_300_block(p, file, 0x10, asc_block, 1, 0x600, bin, 2);
	p += bin2raw_300_block(p, file, 0x00, asc_block, 1, 0x600, bin, 1);

	return (int )(p - raw);
}

/*
	テキストを1200baudベタイメージに変換する (text2rawの下請け)
*/
static int text2raw_1200(unsigned char *raw, const char *file, const char *text, int text_size)
{
	/* 未完成 */
	fprintf(stderr, "Sorry. Not implemented.\n");
	return -1;
}

/*
	テキストをベタイメージに変換する
*/
int text2raw(unsigned char *raw, int baud, const char *file, const char *text, int text_size)
{
	if(baud == 300)
		return text2raw_300(raw, file, text, text_size);
	else if(baud == 1200)
		return text2raw_1200(raw, file, text, text_size);
	else
		return -1;
}

/*
	1ブロック分バイナリを300baudの音声ファイルに変換し, 次のブロックへ移動する (下請け)
*/
static int bin2wave_300_block_next(const char *out_path, const char *file, int kind, int *asc_block, int *bin_block, int *address, const unsigned char **bin, int *bin_size)
{
	int baud = 300, result, raw_size, wave_size;
	unsigned char raw[512], *p_raw = raw, *wave;

	result = bin2raw_300_block_next(&p_raw, file, kind, asc_block, bin_block, address, bin, bin_size);
	raw_size = (int )(p_raw - raw);

	wave = malloc(getWaveSize(baud, raw_size));
	wave_size = putWaveSilent(wave, 1);
	if(appendWave(out_path, wave, wave_size) < 0)
		result = -1;

	wave_size = putWaveBytes(wave, baud, raw, raw_size);
	if(appendWave(out_path, wave, wave_size) < 0)
		result = -1;

	free(wave);
	return result;
}

/*
	1ブロック分バイナリを300baudの音声ファイルに変換する (下請け)
*/
static int bin2wave_300_block(const char *out_path, const char *file, int kind, int asc_block, int bin_block, int address, const unsigned char *bin, int bin_size)
{
	return bin2wave_300_block_next(out_path, file, kind, &asc_block, &bin_block, &address, &bin, &bin_size);
}

/*
	バイナリを300baudの音声ファイルに変換する (bin2waveの下請け)
*/
static int bin2wave_300(const char *out_path, const char *file, int address, const unsigned char *bin, int bin_size)
{
	int asc_block = 1, bin_block = 1, result;

	while((result = bin2wave_300_block_next(out_path, file, 0x01, &asc_block, &bin_block, &address, &bin, &bin_size)) > 0)
		;
	return (result == 0 ? 1: -1);
}

/*
	バイナリを1200baudの音声ファイルに変換する (bin2waveの下請け)
*/
static int bin2wave_1200(const char *out_path, const char *file, int address, const unsigned char *bin, int bin_size)
{
	int baud = 1200, raw_size, wave_size, result;
	unsigned char *raw, *wave;

	raw = malloc(272 + bin_size);
	raw_size = bin2raw(raw, baud, file, address, bin, bin_size);

	wave = malloc(MAX(getWaveSize(baud, raw_size), getWaveSizeSec(1)));
	wave_size = putWaveSilent(wave, 1);
	appendWave(out_path, wave, wave_size);

	wave_size = putWaveBytes(wave, baud, raw, raw_size);
	result = appendWave(out_path, wave, wave_size);

	free(wave);
	return result;
}

/*
	バイナリを音声ファイルに変換する
*/
int bin2wave(const char *out_path, int baud, const char *file, int address, const unsigned char *bin, int bin_size)
{
	if(baud == 300)
		return bin2wave_300(out_path, file, address, bin, bin_size);
	else if(baud == 1200)
		return bin2wave_1200(out_path, file, address, bin, bin_size);
	else
		return -1;
}

/*
	テキストファイルを300baudの音声に変換する (text2wavの下請け)
*/
static int text2wave_300(const char *out_path, const char *file, const char *text, int text_size)
{
	int asc_block = 1, bin_size;
	unsigned char bin[0x100];

	while(text_size > 0) {
		bin_size = text2bin(bin, &text, &text_size);
		bin2wave_300_block(out_path, file, 0x10, asc_block, 1, 0x600, bin, bin_size);
		bin2wave_300_block(out_path, file, 0x00, asc_block, 1, 0x600, bin, 1);
		asc_block++;
	}

	bin[1] = bin[0] = 0xff;
	bin2wave_300_block(out_path, file, 0x10, asc_block, 1, 0x600, bin, 2);
	return bin2wave_300_block(out_path, file, 0x00, asc_block, 1, 0x600, bin, 1);
}

/*
	テキストファイルを1200baudの音声に変換する (text2wavの下請け)
*/
static int text2wave_1200(const char *out_path, const char *file, const char *text, int text_size)
{
	/* 未完成 */
	fprintf(stderr, "Sorry. Not implemented.\n");
	return -1;
}

/*
	テキストファイルを音声に変換する
*/
int text2wave(const char *out_path, int baud, const char *file, const char *text, int text_size)
{
	if(baud == 300)
		return text2wave_300(out_path, file, text, text_size);
	else if(baud == 1200)
		return text2wave_1200(out_path, file, text, text_size);
	else
		return -1;
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
