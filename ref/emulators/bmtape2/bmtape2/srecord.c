/*
	Motorola Sレコード処理 (srecord.c)
*/

#include <unistd.h>
#include <stdio.h>
#include <limits.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "srecord.h"

#define MAX_SIZE	0x7fffffff

/*
	文字を数値(1byte)に変換する (下請け)
*/
static int h2i(char c)
{
	switch(c) {
	case '0':
		return 0;
	case '1':
		return 1;
	case '2':
		return 2;
	case '3':
		return 3;
	case '4':
		return 4;
	case '5':
		return 5;
	case '6':
		return 6;
	case '7':
		return 7;
	case '8':
		return 8;
	case '9':
		return 9;
	case 'A':
	case 'a':
		return 0xa;
	case 'B':
	case 'b':
		return 0xb;
	case 'C':
	case 'c':
		return 0xc;
	case 'D':
	case 'd':
		return 0xd;
	case 'E':
	case 'e':
		return 0xe;
	case 'F':
	case 'f':
		return 0xf;
	default:
		return -16;
	}
}
static int hex2i(const char *hex)
{
	int val;

	if((val = (h2i(*hex) << 4) | h2i(*(hex + 1))) < 0)
		return -1;
	return val;
}

/*
	Sレコード形式のテキストをバイナリに変換する (下請け)
*/
static int s2bin(void *mem, int *off, const char *txt, int mem_size, int check)
{
	int i, address_size, len, val, sum = 0;
	const char *r = txt;
	unsigned char *w;

	*off = 0;

	/* スタートマーク */
	if(*r++ != 'S')
		return -1;

	/* レコードタイプ */
	switch(*r++) {
	case '0':	/* スタートレコード */
		return 0;
	case '1':	/* データレコード(16bit) */
		address_size = 2;
		break;
	case '2':	/* データレコード(24bit) */
		address_size = 3;
		break;
	case '3':	/* データレコード(32bit) */
		address_size = 4;
		break;
	case '4':	/* シンボルレコード */
	case '5':	/* データレコード数(2byte) */
	case '6':	/* データレコード数(3byte) */
		return 0;
	case '7':	/* 終了(32bit) */
	case '8':	/* 終了(24bit) */
	case '9':	/* 終了(16bit) */
		return 0;
	default:	/* 異常 */
		return -1;
	}

	/* レコード桁数 */
	if((val = hex2i(r)) < 0)
		return -1;
	len = val - address_size - 1;
	sum = val;
	r += 2;

	/* アドレス */
	*off = 0;
	for(; address_size > 0; address_size--, r += 2) {
		if((val = hex2i(r)) < 0)
			return -1;
		*off |= (val << (address_size * 8 - 8));
		sum += val;
	}

	/* データ部 */
	for(
	i = 0, w = (unsigned char *)mem + *off;
	i < len && w < (unsigned char *)mem + mem_size;
	i++, w++, r += 2
	) {
		if((val = hex2i(r)) < 0)
			return -1;
		if(mem != NULL)
			*w = val;
		sum += val;
	}

	/* チェックサム */
	if((val = hex2i(r)) < 0)
		return -1;
	sum += val;
	if(check && (sum & 0xff) != 0xff)
		return -1;

	return (mem != NULL ? w - (unsigned char *)mem - *off: len);
}

/*
	数値(1byte)を文字に変換する (下請け)
*/
static char *int2c(char *hex, int i)
{
	const static char int_to_ascii[] = {
		'0', '1', '2', '3', '4', '5', '6', '7',
		'8', '9', 'A', 'B', 'C', 'D', 'E', 'F'
	};

	hex[0] = int_to_ascii[i >> 4];
	hex[1] = int_to_ascii[i & 0xf];
	return hex;
}

/*
	バイナリをSレコード形式のテキストに変換する (下請け)
*/
static char *bin2s(char *txt, const unsigned char *mem, int off, int len, char record_type)
{
	int sum;
	const unsigned char *r;
	char *w = txt;

	/* スタートマーク */
	*w++ = 'S';

	/* レコードタイプ */
	if(len > 0)
		*w = record_type;
	else if(record_type == '3')
		*w = '7';
	else if(record_type == '2')
		*w = '8';
	else if(record_type == '1')
		*w = '9';
	w++;

	/* レコード長 */
	int2c(w, len + 3);
	sum = len + 3;
	w += 2;

	/* アドレス */
	if(record_type == '3') {
		int2c(w, (off & 0xff000000) >> 24);
		w += 2;
		sum += ((off & 0xff000000) >> 24);
	}
	if(record_type == '3' || record_type == '2') {
		int2c(w, (off & 0x00ff0000) >> 16);
		w += 2;
		sum += ((off & 0x00ff0000) >> 16);
	}
	int2c(w, (off & 0x0000ff00) >> 8);
	w += 2;
	sum += ((off & 0x0000ff00) >> 8);
	int2c(w, off & 0x000000ff);
	w += 2;
	sum += (off & 0x000000ff);

	/* データ部 */
	if(mem != NULL)
		for(r = mem + off; r < mem + off + len; r++, w += 2) {
			int2c(w, *r);
			sum += *r;
		}

	/* チェックサム */
	int2c(w, ~sum & 0xff);
	w += 2;

	/* 改行 */
	*w++ = '\n';
	*w = 0;

	return txt;
}

/*
	Sレコード形式のファイルを読み込む
*/
int readSRecord(const char *path, void *mem, int *ret_begin, int mem_size, int check)
{
	FILE *fp;
	int len, off, begin = MAX_SIZE, last = 0;
	char buf[256];

	if((fp = fopen(path, "r")) == NULL)
		return -1;
	while(!feof(fp)) {
		fgets(buf, sizeof(buf), fp);
		len = s2bin(mem, &off, buf, mem_size, check);
		if(len < 0 || len == sizeof(buf)) {
			fclose(fp);
			return -1;
		} else if(len > 0) {
			begin = (begin < off ? begin: off);
			last = (last > off + len ? last: off + len);
		}
	}
	fclose(fp);

	if(ret_begin != NULL)
		*ret_begin = begin;
	return (begin > last ? 0: last - begin);
}

/*
	Sレコード形式のファイルを読み込む(ファイル内のアドレスを無視する)
*/
int readSRecordAbs(const char *path, void *mem, int *ret_begin, int mem_size, int check)
{
	int size, begin;

	if((size = readSRecord(path, NULL, &begin, MAX_SIZE, check)) == 0)
		return -1;
	if(ret_begin != NULL)
		*ret_begin = begin;
	return readSRecord(path, (mem != NULL ? (unsigned char *)mem - begin: NULL), NULL, mem_size + begin, check);
}

/*
	Sレコード形式のファイルを書き込む (開いたファイルに対して)
*/
static int putSRecord(int fd, const void *mem, int begin, int size, char record_type)
{
	char buf[256];
	int off, len;

	if(size <= 0) {
		bin2s(buf, mem, begin, size, record_type);
		return write(fd, buf, strlen(buf));
	}

	for(off = begin; off < begin + size; off += 16) {
		if((len = begin + size - off) > 16)
			len = 16;
		bin2s(buf, mem, off, len, record_type);
		write(fd, buf, strlen(buf));
	}

	return 0;
}

/*
	Sレコード形式のファイルを書き込む
*/
int writeSRecord(const char *path, const void *mem, int begin, int size)
{
	int fd;
	char record_type;

	if((fd = open(path, O_CREAT | O_TRUNC | O_WRONLY, 0664)) < 0)
		return 0;
		
	if(begin + size <= 0x0000ffff)
		record_type = '1';
	else if(begin + size <= 0x00ffffff)
		record_type = '2';
	else 
		record_type = '3';

	if(
	putSRecord(fd, mem, begin, size, record_type) < 0 ||
	putSRecord(fd, NULL, begin + size, 0, record_type) < 0
	) {
		close(fd);
		return 0;
	}

	close(fd);
	return size;
}

/*
	Sレコード形式のファイルを書き込む(メモリの先頭から書き込む)
*/
int writeSRecordAbs(const char *path, const void *mem, int begin, int size)
{
	return writeSRecord(path, (unsigned char *)mem - begin, begin, size);
}

/*
	Sレコード形式のファイルに追加する
*/
int appendSRecord(const char *path, const void *mem, int begin, int size)
{
	int fd;
	char record_type, start;

	if((fd = open(path, O_RDWR)) < 0)
		return writeSRecord(path, mem, begin, size);

	lseek(fd, 1, SEEK_SET);
	read(fd, &record_type, 1);
	
	lseek(fd, -12, SEEK_END);
	read(fd, &start, 1);
	if(start == 'S')
		lseek(fd, -1, SEEK_CUR);
	putSRecord(fd, mem, begin, size, record_type);
	putSRecord(fd, NULL, begin + size, 0, record_type);

	close(fd);
	return 0;
}

/*
	Sレコード形式のファイルに追加する(メモリの先頭から書き込む)
*/
int appendSRecordAbs(const char *path, const void *mem, int begin, int size)
{
	return appendSRecord(path, (unsigned char *)mem - begin, begin, size);;
}

/*
	Copyright 2008 maruhiro
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
