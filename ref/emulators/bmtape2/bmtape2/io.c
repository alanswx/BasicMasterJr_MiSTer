/*
	bmtape2
	入出力
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "bmtape2.h"

#define FALSE	0
#define TRUE	1

#ifndef O_BINARY
#	define O_BINARY	0
#endif

/*
	ディスク上にリトルエンディアンで書き込まれた2バイトの数値を読み込む
*/
static int read2(int fd, int *val)
{
	int success;
	unsigned char buf[2];

	success = read(fd, buf, 2);
	*val = buf[0] + buf[1] * 0x100;
	return success;
}

/*
	ディスクに2バイトをリトルエンディアンで書き込む
*/
static int write2(int fd, int val)
{
	unsigned char buf[2];

	buf[0] = val & 0xff;
	buf[1] = (val >> 8) & 0xff;
	return write(fd, buf, 2);
}

/*
	ディスク上にリトルエンディアンで書き込まれた4バイトの数値を読み込む
*/
static int read4(int fd, int *val)
{
	int success;
	unsigned char buf[4];

	success = read(fd, buf, 4);
	*val = buf[0] + buf[1] * 0x100 + buf[2] * 0x10000 + buf[3] * 0x1000000;
	return success;
}

/*
	ディスクに4バイトをリトルエンディアンで書き込む
*/
static int write4(int fd, int val)
{
	unsigned char buf[4];

	buf[0] = val & 0xff;
	buf[1] = (val >> 8) & 0xff;
	buf[2] = (val >> 16) & 0xff;
	buf[3] = (val >> 24) & 0xff;
	return write(fd, buf, 4);
}

/*
	16bitの音声を8bitに変換する
*/
static int convBit16to8(unsigned char *wave, int size)
{
	unsigned char *r, *w;

	for(r = wave, w = wave; r < wave + size; r += 2, w++)
		*w = ((short )(*(r + 0) + *(r + 1) * 0x100) + 32767) / 256;
	return size / 2;
}

/*
	ステレオの音声をモノラルにする
*/
static int convChannel2to1(unsigned char *wave, int size)
{
	unsigned char *r, *w;

	for(r = wave, w = wave; r < wave + size; r += 2)
		*w = (*(r + 0) + *(r + 1)) / 2;
	return size / 2;
}

/*
	WAVEファイルか?
*/
int isWave(const char *path)
{
	int fd;
	char buf[256];

	if((fd = open(path, O_RDONLY | O_BINARY)) < 0)
		return FALSE;
	read(fd, buf, 12);
	close(fd);

	return memcmp(buf, "RIFF", 4) == 0 && memcmp(buf + 8, "WAVE", 4) == 0;
}

/*
	WAVE形式の音声ファイルを読み込む
*/
void *readWave(int *data_size, int *rate, const char *path)
{
	int fd, size, fmt, dummy, bits = 8, channels = 1;
	char buf[256];
	unsigned char *wave = NULL;

	if((fd = open(path, O_RDONLY | O_BINARY)) < 0)
		return NULL;

	read(fd, buf, 4); /* RIFFヘッダ */
	if(memcmp(buf, "RIFF", 4) != 0)
		goto fail;
	read4(fd, &size); /* ファイルサイズ */

	read(fd, buf, 4); /* WAVE */
	if(memcmp(buf, "WAVE", 4) != 0)
		goto fail;

	for(;;) {
		if(read(fd, buf, 4) <= 0) /* チャンクID */
			break;
		if(read4(fd, &size) <= 0) /* チャンクサイズ */
			break;

		if(memcmp(buf, "fmt ", 4) == 0) {
			read2(fd, &fmt); /* フォーマットID */
			if(fmt != 1)
				goto fail;
			read2(fd, &channels); /* チャネル数 */
			read4(fd, rate); /* サンプリングレート */
			read4(fd, &dummy); /* データ速度(無視する) */
			read2(fd, &dummy); /* ブロックサイズ(無視する) */
			read2(fd, &bits); /* サンプルあたりのbit数 */
			if(size > 0)
				read(fd, buf, size - 16); /* 拡張部分(無視する) */
		} else if(memcmp(buf, "data", 4) == 0) {
			*data_size = size;
			if((wave = malloc(*data_size)) == NULL)
				goto fail;
			*data_size = read(fd, wave, *data_size);
			if(bits == 16)
				*data_size = convBit16to8(wave, *data_size);
			if(channels == 2)
				*data_size = convChannel2to1(wave, *data_size);
			wave = realloc(wave, *data_size);
			break;
		} else
			lseek(fd, size, SEEK_CUR);
	}

	close(fd);
	return wave;

fail:;
	if(wave != NULL)
		free(wave);
	close(fd);
	return NULL;
}

/*
	音声をWAVE形式で書き込む
*/
int writeWave(const char *path, unsigned char *data, int data_size)
{
	int fd;

	if((fd = open(path, O_CREAT | O_TRUNC | O_WRONLY | O_BINARY, 0664)) < 0)
		return -1;

	/* RIFFヘッダ */
	write(fd, "RIFF", 4);
	write4(fd, 36 + data_size);

	/* WAVEヘッダ */
	write(fd, "WAVE", 4);

	/* fmtチャンク */
	write(fd, "fmt ", 4);
	write4(fd, 16);
	write2(fd, 1);
	write2(fd, 1);
	write4(fd, 44100);
	write4(fd, 44100);
	write2(fd, 1);
	write2(fd, 8);

	/* dataチャンク */
	write(fd, "data", 4);
	write4(fd, data_size);
	write(fd, data, data_size);

	close(fd);
	return 32 + data_size;
}

/*
	音声をWAVEファイルに追加する
*/
int appendWave(const char *path, unsigned char *append, int append_size)
{
	struct stat stat;
	int fd, size, data_size, val;
	char buf[256];

	if((fd = open(path, O_RDONLY | O_BINARY)) < 0)
		return writeWave(path, append, append_size);

	fstat(fd, &stat);
	if(stat.st_size == 0) {
		close(fd);
		return writeWave(path, append, append_size);
	}

	/* RIFFヘッダ */
	read(fd, buf, 4);
	if(memcmp(buf, "RIFF", 4) != 0)
		goto fail;

	/* サイズ */
	read4(fd, &size);

	/* WAVEヘッダ */
	read(fd, buf, 4);
	if(memcmp(buf, "WAVE", 4) != 0)
		goto fail;

	/* fmtチャンク */
	read(fd, buf, 4);
	if(memcmp(buf, "fmt ", 4) != 0)
		goto fail;
	read4(fd, &val);
	if(val != 16)
		goto fail;
	read2(fd, &val);
	if(val != 1)
		goto fail;
	read2(fd, &val);
	if(val != 1)
		goto fail;
	read4(fd, &val);
	if(val != 44100)
		goto fail;
	read4(fd, &val);
	if(val != 44100)
		goto fail;
	read2(fd, &val);
	if(val != 1)
		goto fail;
	read2(fd, &val);
	if(val != 8)
		goto fail;

	/* dataチャンク */
	read(fd, buf, 4);
	if(memcmp(buf, "data", 4) != 0)
		goto fail;
	read4(fd, &data_size);
	close(fd);

	if((fd = open(path, O_WRONLY | O_BINARY)) < 0)
		return -1;

	/* サイズ */
	lseek(fd, 4, SEEK_SET);
	write4(fd, size + append_size);

	/* dataチャンク */
	lseek(fd, 40, SEEK_SET);
	write4(fd, data_size + append_size);

	/* 追加データ */
	lseek(fd, 0, SEEK_END);
	write(fd, append, append_size);
	close(fd);
	return size + append_size;

fail:;
	close(fd);
	return -1;
}

/*
	ファイルを読み込む(下請け)
*/
static int readFile(const char *path, void *mem, int mem_size, int flag)
{
	int fd, size;

	if((fd = open(path, O_RDONLY | flag)) < 0)
		return -1;
	size = read(fd, mem, mem_size);
	close(fd);

	return size;
}

/*
	ファイルに書き込む(下請け)
*/
static int writeFile(const char *path, const void *mem, int mem_size, int flag)
{
	int fd, written_size;

	if((fd = open(path, O_CREAT | O_TRUNC | O_WRONLY | flag, 0664)) < 0)
		return -1;
	if(mem != NULL)
		written_size = write(fd, mem, mem_size);
	else
		written_size = 0;
	close(fd);
	
	return written_size;
}

/*
	ファイルに追加する(下請け)
*/
int appendFile(const char *path, const void *append, int append_size, int flag)
{
	int fd, written_size;
	struct stat stat;

	if((fd = open(path, O_WRONLY | flag)) < 0)
		return writeBin(path, append, append_size);
	fstat(fd, &stat);
	lseek(fd, 0, SEEK_END);
	if(append != NULL)
		written_size = write(fd, append, append_size);
	else
		written_size = 0;
	close(fd);

	return stat.st_size + append_size;
}

/*
	バイナリファイルを読み込む
*/
int readBin(const char *path, void *mem, int mem_size)
{
	return readFile(path, mem, mem_size, O_BINARY);
}

/*
	バイナリファイルを書き込む
*/
int writeBin(const char *path, const void *mem, int mem_size)
{
	return writeFile(path, mem, mem_size, O_BINARY);
}

/*
	バイナリファイルに追加する
*/
int appendBin(const char *path, const void *append, int append_size)
{
	return appendFile(path, append, append_size, O_BINARY);
}

/*
	テキストファイルを読み込む
*/
int readText(const char *path, void *mem, int mem_size)
{
	return readFile(path, mem, mem_size, 0);
}

/*
	テキストファイルを書き込む
*/
int writeText(const char *path, const void *mem, int mem_size)
{
	return writeFile(path, mem, mem_size, 0);
}

/*
	テキストファイルに追加する
*/
int appendText(const char *path, const void *append, int append_size)
{
	return appendFile(path, append, append_size, 0);
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
