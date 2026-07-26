/*
	bmtape2
	カンサスシティスタンダード・ベタイメージ相互変換
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include "bmtape2.h"

#define FALSE	0
#define TRUE	1
#define WAVE_MIN	4800

#if 0
/*
	2つの値を比較する (filtMedの下請け)
*/
static int cmpval(const void *a, const void *b)
{
	return (int )(*(unsigned char *)a) - (int )(*(unsigned char *)b);
}

/*
	中央値フィルタを適用する (wave2rawの下請け)
*/
static int filtMed(unsigned char *wave, int size, int width)
{
	unsigned char *p, *q, *r, val[256];

	for(p = wave; p < wave + size - width; p++) {
		for(q = p, r = val; q < p + width; q++, r++)
			*r = *q;
		qsort(val, width, sizeof(unsigned char), cmpval);
		*p = val[width / 2];
	}

	return size - width;
}
#endif

/*
	平均値フィルタを適用する (wave2rawの下請け)
*/
static int filtSmooth(unsigned char *wave, int size, int width)
{
	int i;
	unsigned char *p;

	for(i = 0; i < width - 1; i++) {
		for(p = wave; p < wave + size - width - 1; p++)
			*p = ((int )*(p + 0) + (int )*(p + 1)) / 2;
	}

	return size - width - 1;
}

/*
	波形の平均値を得る (wave2rawの下請け)
*/
static int getMid(unsigned char *mid, const unsigned char *wave, int size, int width)
{
	int i, sum;
	unsigned char *p = mid;

	if(width == 0) {
		memset(mid, 0x7f, size);
		return size;
	}

	/* 最初の平均値を得る */
	sum = 0;
	for(i = 0; i < width - 1; i++)
		sum += wave[i];
	*p++ = sum / width;

	/* 以降の平均値を得る */
	for(i = 1; i < size - width - 1; i++) {
		sum = sum - wave[i - 1] + wave[i + width];
		*p++ = MIN(MAX(sum, 0) / width, 255);
	}

	return size - width;
}

/*
	立ち上がりか? (_getLengthの下請け)
*/
static inline int isUp(const unsigned char *wave, const unsigned char *mid)
{
	return *wave <= *mid && *(wave + 1) > *(mid + 1);
}

/*
	立ち下がりか? (_getLengthの下請け)
*/
static inline int isDown(const unsigned char *wave, const unsigned char *mid)
{
	return *wave >= *mid && *(wave + 1) < *(mid + 1);
}

/*
	(半)波長を得る. ポインタを次に進める (下請け)
*/
static int _getLength(const unsigned char **wave, const unsigned char **mid, int *size, int *ret_up_down, int up_down)
{
	int len, ret = 0;
	const unsigned char *p, *q;

	if(up_down == 0) {
		for(p = *wave, q = *mid; p < *wave + *size - 1; p++, q++)
			if(isDown(p, q)) {
				ret = 0;
				break;
			}
	} else if(up_down == 1) {
		for(p = *wave, q = *mid; p < *wave + *size - 1; p++, q++)
			if(isUp(p, q)) {
				ret = 1;
				break;
			}
	} else {
		for(p = *wave, q = *mid; p < *wave + *size - 1; p++, q++)
			if(isDown(p, q)) {
				ret = 0;
				break;
			} else if(isUp(p, q)) {
				ret = 1;
				break;
			}
	}

	len = (int )(p - *wave) + 1;
	*wave += len;
	*mid += len;
	*size -= len;

	if(*size <= 0)
		return -1;
	if(ret_up_down != NULL)
		*ret_up_down = ret;
	return (len < SHRT_MAX ? len: SHRT_MAX);
}

/*
	波長を得る. ポインタを次に進める (下請け)
*/
static int getLength(const unsigned char **wave, const unsigned char **mid, int *size, int up_down)
{
	return _getLength(wave, mid, size, NULL, up_down);
}

/*
	半波長を得る. ポインタを次に進める (下請け)
*/
static int getHalfLength(const unsigned char **wave, const unsigned char **mid, int *size, int *up_down)
{
	return _getLength(wave, mid, size, up_down, -1);
}

/*
	半波長を得る. ポインタを次に進めない (下請け)
*/
static int peekHalfLength(const unsigned char *wave, const unsigned char *mid, int size, int *up_down)
{
	const unsigned char *wave0 = wave, *mid0 = mid;
	
	return _getLength(&wave0, &mid0, &size, up_down, -1);
}

/*
	高音部, 低音部の波長を得る (wave2rawの下請け)
*/
static void getHighLow(int *high, int *low, int *min, int rate, const unsigned char *wave, const unsigned char *mid, int size)
{
	int len, min_max, n[32];

	memset(n, 0, sizeof(n));

	/* 半波長のヒストグラムを得る */
	while((len = getHalfLength(&wave, &mid, &size, NULL)) >= 0)
		n[MIN(len, sizeof(n) / sizeof(n[0]) - 1)]++;

	/* ヒストグラムを平滑化する */
	for(len = 0; len < sizeof(n) / sizeof(n[0]) - 1; len++)
		n[len] += n[len + 1];
	for(len = sizeof(n) / sizeof(n[0]) - 1; len > 0; len--)
		n[len] += n[len - 1];
#if 0
	for(len = 1; len < sizeof(n) / sizeof(n[0]); len++)
		fprintf(stderr, "%d\t%dHz\t%d\n", len, rate / 2 / len, n[len]);
#endif

	/* 低音部の周波数を得る */
	min_max = -1;
	for(len = rate / 2 / 1500; len < rate / 2 / 900; len++)
		if(min_max < n[len]) {
			*low = len;
			min_max = n[len];
		}

	/* 高音部の周波数を得る */
	min_max = -1;
	for(len = rate / 2 / 3000; len < rate / 2 / 1800; len++)
		if(min_max < n[len]) {
			*high = len;
			min_max = n[len];
		}

	/* 最も少ない周波数を得る(参考) */
	min_max = INT_MAX;
	for(len = *high; len < *low; len++)
		if(min_max > n[len]) {
			*min = len;
			min_max = n[len];
		}

	*low *= 2;
	*high *= 2;
	*min *= 2;
}

/*
	無音部を読み飛ばす (wave2rawの下請け)
*/
static int _skipSilent(const unsigned char *wave, const unsigned char *mid, int size, int th_high, int th_low)
{
	int len;

	if((len = peekHalfLength(wave, mid, size, NULL)) < 0)
		return -1;
	if(len <= th_high / 2 || len >= th_low / 2)
		return len;
	if(size - len <= 0)
		return -1;

	len += peekHalfLength(wave + len, mid + len, size - len, NULL);
	if(len >= th_low)
		return len;
	return 0;
}
static int skipSilent(const unsigned char **wave, const unsigned char **mid, int *size, int th_high, int th_low)
{
	int l, len = 0;

	while((l = _skipSilent(*wave + len, *mid + len, *size - len, th_high, th_low)) > 0)
		len += l;
	if(l < 0)
		return -1;
	if(len <= 0)
		return len;

	*wave += len;
	*mid += len;
	*size -= len;

	return len;
}

/*
	波が立ち上がりで始まるか, 立ち下がりで始まるか? (wave2rawの下請け)
*/
static int getUpOrDown(const unsigned char *wave, const unsigned char *mid, int size, int th_low)
{
	int up_down, len, prev = 0, err_up = 0, err_down = 0;

	while(len = getHalfLength(&wave, &mid, &size, &up_down), 0 < len && len < th_low) {
		if(prev > 0) {
			if(up_down == 0)
				err_up += abs(len - prev);
			else if(up_down == 1)
				err_down += abs(len - prev);
		}
		prev = len;
	}

	return (err_up > err_down ? 0: 1);
}

/*
	波形から波長を得る (wave2rawの下請け)
*/
static void *allocLength(int *ret_size, const unsigned char **wave, const unsigned char **mid, int *size, int up_down, int th_high, int th_low)
{
	const unsigned char *wave0 = *wave, *mid0 = *mid;
	short *length, *p;
	int size0 = *size, i, len, n = 0;

	/* メモリを確保する */
	while(len = getLength(&wave0, &mid0, &size0, up_down), len > 0 && len < th_low)
		n++;
	if(n == 0)
		n = 1;
	p = length = calloc(n + 64, sizeof(short));

	/* 波長を得る */
	for(i = 0; i < n; i++)
		*p++ = (short )getLength(wave, mid, size, up_down);

	*ret_size = (int )(p - length);
	return length;
}

/*
	波長からビットを得る. ポインタを進めない (getByteの下請け)
*/
static int peekBit1(int *bit, short *length, int th_low, int th_zero)
{
	if(length[0] >= th_low) {
		*bit = -1;
		return 1;
	}

	if(length[0] > th_zero) {
		*bit = 0;
		return 1;
	} else if(length[0] <= th_zero && length[1] <= th_zero) {
		*bit = 1;
		return 2;
	} else if(length[1] - length[0] > th_zero) {
		*bit = 1;
		return 1;
	} else {
		*bit = 1;
		return 1; /* 2がよいか? */
	}
}
static int peekBit2(int *bit, short *length, int th_low, int th_zero)
{
	short *p = length;
	int b[2];

	p += peekBit1(&b[0], p, th_low, th_zero);
	p += peekBit1(&b[1], p, th_low, th_zero);

	if(b[0] == 0 && b[1] == 0)
		*bit = 0;
	else if(bit[0] == 1 && bit[1] == 1)
		*bit = 1;
	else {
		*bit = -1;
		return 1;
	}

	return (int )(p - length);
}
static int peekBit4(int *bit, short *length, int th_low, int th_zero)
{
	short *p = length, *q;
	int b[4];

	p += peekBit1(&b[0], p, th_low, th_zero);
	p += peekBit1(&b[1], p, th_low, th_zero);
	p += peekBit1(&b[2], p, th_low, th_zero);
	q = p;
	p += peekBit1(&b[3], p, th_low, th_zero);

	if(b[0] == 0 && b[1] == 0 && b[2] == 0 && b[3] == 0) {
		*bit = 0;
		return (int )(p - length);
	} else if(b[0] == 1 && b[1] == 1 && b[2] == 1 && b[3] == 1) {
		*bit = 1;
		return (int )(p - length);
	} else if(b[0] == 0 && b[1] == 0 && b[2] == 0) {
		*bit = 0;
		return (int )(q - length);
	} else if(b[0] == 1 && b[1] == 1 && b[2] == 1) {
		*bit = 1;
		return (int )(q - length);
	} else {
		*bit = -1;
		return 1;
	}
}
static int peekBit(int *bit, int baud, short *length, int th_low, int th_zero)
{
	if(baud == 300)
		return peekBit4(bit, length, th_low, th_zero);
	else if(baud == 600)
		return peekBit2(bit, length, th_low, th_zero);
	else if(baud == 1200)
		return peekBit1(bit, length, th_low, th_zero);
	else {
		*bit = -1;
		return 1;
	}
}

/*
	波長から1バイト得る. ポインタを進める (writeBytesの下請け)
*/
static int getByte(int baud, short **length, int *size, int th_low, int th_zero)
{
	short *p = *length;
	int i, bit[8], start_stop_bit;

	if(*size <= 0)
		return -1;

	/* スタートビット */
	do {
		p += peekBit(&start_stop_bit, baud, p, th_low, th_zero);
		if(start_stop_bit < 0 || p >= *length + *size) {
			(*size)--;
			(*length)++;
			return -1;
		}
	} while(start_stop_bit != 0);

	/* データビット */
	for(i = 0; i < 8; i++) {
		p += peekBit(&bit[i], baud, p, th_low, th_zero);
		if(bit[i] < 0 || p >= *length + *size) {
			(*size)--;
			(*length)++;
			return -1;
		}
	}

	/* ストップビット */
	while(p >= *length + *size && *p <= th_zero)
		p++;

#if 0
	printf("%d%d%d%d%d%d%d%d\t%02x\t%c\n",
	bit[7],
	bit[6],
	bit[5],
	bit[4],
	bit[3],
	bit[2],
	bit[1],
	bit[0],
	(bit[0] ? 0x01: 0) |
	(bit[1] ? 0x02: 0) |
	(bit[2] ? 0x04: 0) |
	(bit[3] ? 0x08: 0) |
	(bit[4] ? 0x10: 0) |
	(bit[5] ? 0x20: 0) |
	(bit[6] ? 0x40: 0) |
	(bit[7] ? 0x80: 0),
	(bit[0] ? 0x01: 0) |
	(bit[1] ? 0x02: 0) |
	(bit[2] ? 0x04: 0) |
	(bit[3] ? 0x08: 0) |
	(bit[4] ? 0x10: 0) |
	(bit[5] ? 0x20: 0) |
	(bit[6] ? 0x40: 0) |
	(bit[7] ? 0x80: 0)
	);
	fflush(stdout);
#endif

	*size -= (int )(p - *length);
	*length = p;
	return
	(bit[0] ? 0x01: 0) |
	(bit[1] ? 0x02: 0) |
	(bit[2] ? 0x04: 0) |
	(bit[3] ? 0x08: 0) |
	(bit[4] ? 0x10: 0) |
	(bit[5] ? 0x20: 0) |
	(bit[6] ? 0x40: 0) |
	(bit[7] ? 0x80: 0);
}

/*
	波長からバイト列を得る (wave2rawの下請け)
*/
static int writeBytes(unsigned char *raw, int baud, short *length, int size, int th_low, int th_zero)
{
	int byte, n;
	unsigned char *p = raw;

	while(size > 0) {
		if((byte = getByte(baud, &length, &size, th_low, th_zero)) >= 0)
			*p++ = (unsigned char )byte;
	}

	n = (int )(p - raw);
	return n;
}

/*
	波形を表示する (wave2rawの下請け)
*/
static void dispWave(int wave_rate, const unsigned char *wave, const unsigned char *mid, int wave_size, int up_down, int th_high, int th_low, int th_zero)
{
	int prev = 0, len;
	const unsigned char *p, *q;

	for(p = wave, q = mid; p < wave + wave_size - 1; p++, q++) {
		if(*p < *q)
			printf("%*s%*s\n", *p / 8, "*", ((int )*q - (int )*p - 1) / 8, "|");
		else if(*p > *q)
			printf("%*s%*s\n", *q / 8, "|", ((int )*p - (int )*q - 1) / 8, "*");
		else
			printf("%*s\n", *q / 8, "*");

		if(
		(up_down == 0 && isDown(p, q)) ||
		(up_down == 1 && isUp(p, q))
		) {
			len = (int )(p - wave) - prev;
			if(len >= th_low)
				break;
			printf(
			"-------------------------------- %d(%d Hz) %d\n",
			(int )(p - wave),
			wave_rate / ((int )(p - wave) - prev),
			((int )(p - wave) - prev) <= th_zero
			);
			prev = (int )(p - wave);
		}
	}

	printf("~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~ ~\n");
}

/*
	音声ファイルからベタイメージを得る
*/
int wave2raw(unsigned char *raw, const char *wave_path, int baud, int smooth_width, int mid_width, int f_zero, int disp_wave)
{
	int wave_size, wave_rate, high, low, min, up_down;
	int th_high, th_low, th_zero;
	int length_size;
	short *length;
	unsigned char *wave, *mid, *p_raw;
	const unsigned char *p_wave, *p_mid;
	
	/* 音声ファイルを読み込む */
	p_wave = wave = readWave(&wave_size, &wave_rate, wave_path);
	if(wave == NULL) {
		fprintf(stderr, "Cannot open %s\n", wave_path);
		return -1;
	}
	smooth_width = (smooth_width * wave_rate + 44100 / 2) / 44100;
	wave_size = filtSmooth(wave, wave_size, smooth_width);
	
	/* 波形の平均値を得る */
	p_mid = mid = malloc(wave_size);
	wave_size = getMid(mid, wave, wave_size, (mid_width == 0 ? 0: wave_rate / mid_width));
	
	/* 閾値を求める */
	getHighLow(&high, &low, &min, wave_rate, wave, mid, wave_size);
	high = low / 2;
	th_high = high / 2;
	th_low = low * 2;
	th_zero = high + (low - high) / 2;
	if(f_zero > 0)
		th_zero = wave_rate / f_zero;
	/*
	fprintf(stderr, "smooth=%d mid=%dHz, high=%dHz, low=%dHz, min=%dHz, t=%dHz\n", smooth_width, mid_width, wave_rate / high, wave_rate / low, wave_rate / min, wave_rate / th_zero);
	*/

	/* ベタデータを得る */
	p_raw = raw;
	while(skipSilent(&p_wave, &p_mid, &wave_size, th_high, th_low) >= 0) {
		up_down = getUpOrDown(p_mid, p_wave, wave_size, th_low);
		if(disp_wave)
			dispWave(wave_rate, p_wave, p_mid, wave_size, up_down, th_high, th_low, th_zero);
		length = allocLength(&length_size, &p_wave, &p_mid, &wave_size, up_down, th_high, th_low);
		p_raw += writeBytes(p_raw, baud, length, length_size, th_low, th_zero);
		free(length);
	}

	free(mid);
	free(wave);
	return (int )(p_raw - raw);
}

/*
	音声を半波長書き込む (putWaveBitの下請け)
*/
static int putWave(unsigned char *p, int value)
{
	static int e = 0;
	int size = 44100 / WAVE_MIN + (e >= WAVE_MIN ? 1: 0);

	memset(p, value, size);
	if(e >= WAVE_MIN)
		e -= WAVE_MIN;
	e += 44100 % WAVE_MIN;

	return size;
}

/*
	音声を1ビット書き込む (putWaveByteの下請け)
*/
static int putWaveBit(unsigned char *p, int baud, int bit)
{
	int i;
	unsigned char *begin = p;

	for(i = 0; i < 1200 / baud; i++)
		if(bit) {
			p += putWave(p, 0xff);
			p += putWave(p, 0x00);
			p += putWave(p, 0xff);
			p += putWave(p, 0x00);
		} else {
			p += putWave(p, 0xff);
			p += putWave(p, 0xff);
			p += putWave(p, 0x00);
			p += putWave(p, 0x00);
		}

	return (int )(p - begin);
}

/*
	音声を1バイト書き込む (putWaveBytesの下請け)
*/
int putWaveByte(unsigned char *p, int baud, unsigned char data)
{
	unsigned char *begin = p;

	p += putWaveBit(p, baud, 0);
	p += putWaveBit(p, baud, data & 0x01);
	p += putWaveBit(p, baud, data & 0x02);
	p += putWaveBit(p, baud, data & 0x04);
	p += putWaveBit(p, baud, data & 0x08);
	p += putWaveBit(p, baud, data & 0x10);
	p += putWaveBit(p, baud, data & 0x20);
	p += putWaveBit(p, baud, data & 0x40);
	p += putWaveBit(p, baud, data & 0x80);
	p += putWaveBit(p, baud, 1);
	p += putWaveBit(p, baud, 1);

	return (int )(p - begin);
}

/*
	音声のサイズを得る(バイト数)
*/
int getWaveSize(int baud, int data_size)
{
	return (data_size * 11) * (44100 / 4800 + 1) * (1200 / baud) * 4;
}

/*
	音声にバイト列を書き込む
*/
int putWaveBytes(unsigned char *wave, int baud, const unsigned char *data, int data_size)
{
	const unsigned char *p_data;
	unsigned char *p_wave = wave;

	for(p_data = data; p_data < data + data_size; p_data++)
		p_wave += putWaveByte(p_wave, baud, *p_data);

	return (int )(p_wave - wave);
}

/*
	音声のサイズを得る(秒数)
*/
int getWaveSizeSec(int sec)
{
	return sec * 44100;
}

/*
	無音を書き込む
*/
int putWaveSilent(unsigned char *wave, int sec)
{
	int size = getWaveSizeSec(sec);

	memset(wave, 0, size);
	return size;
}

/*
	ベタイメージを音声ファイルに変換する
*/
int raw2wave(const char *out_path, int baud, const unsigned char *raw, int raw_size)
{
	int wave_size, result;
	unsigned char *wave;

	wave = malloc(getWaveSize(baud, raw_size));
	wave_size = putWaveBytes(wave, baud, raw, raw_size);
	result = appendWave(out_path, wave, wave_size);
	free(wave);

	return result;
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
