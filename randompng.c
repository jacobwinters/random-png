// This file by Jacob Winters; released under CC0

#include <stdlib.h>
#include <string.h>

typedef unsigned char u8;

int puff(u8 *dest, unsigned long *destlen, const u8 *source, unsigned long *sourcelen);
void png_read_filter_row(size_t rowbytes, u8 pixel_depth, u8 *row, const u8 *prev_row, int filter);

#ifdef __EMSCRIPTEN__
	#include <emscripten.h>
	#define GETENTROPY_MAXBYTES 65536
	EM_JS(void, getentropy, (void* ptr, size_t length), {
		crypto.getRandomValues(HEAP8.subarray(ptr, ptr + length));
	});
#else
	#include <sys/random.h>
	#define GETENTROPY_MAXBYTES 256
#endif

void fillrandbuffer(void *buffer, size_t length) {
	while (length > GETENTROPY_MAXBYTES) {
		getentropy(buffer, GETENTROPY_MAXBYTES);
		buffer += GETENTROPY_MAXBYTES;
		length -= GETENTROPY_MAXBYTES;
	}
	getentropy(buffer, length);
}

u8 randombyte() {
	static u8 buffer[GETENTROPY_MAXBYTES];
	static int bytesUsed = GETENTROPY_MAXBYTES;
	if (bytesUsed == GETENTROPY_MAXBYTES) {
		getentropy(buffer, GETENTROPY_MAXBYTES);
		bytesUsed = 0;
	}
	return buffer[bytesUsed++];
}

u8 randombytebelow(u8 max) {
	u8 result;
	do {
	  result = randombyte();
	} while (result >= max);
	return result;
}

void *growrandbuffer(void *buffer, size_t oldLength, size_t newLength) {
	buffer = realloc(buffer, newLength);
	fillrandbuffer(buffer + oldLength, newLength - oldLength);
	return buffer;
}

void copyrgbtorgba(int pixelCount, u8 *rgbBuffer, u8 *rgbaBuffer) {
	for (int i = 0; i < pixelCount; i++) {
		*rgbaBuffer++ = *rgbBuffer++;
		*rgbaBuffer++ = *rgbBuffer++;
		*rgbaBuffer++ = *rgbBuffer++;
		*rgbaBuffer++ = (u8)255;
	}
}

void copygrayscaletorgba(int pixelCount, u8 *grayBuffer, u8 *rgbaBuffer) {
	for (int i = 0; i < pixelCount; i++) {
		*rgbaBuffer++ = *grayBuffer;
		*rgbaBuffer++ = *grayBuffer;
		*rgbaBuffer++ = *grayBuffer++;
		*rgbaBuffer++ = (u8)255;
	}
}

void pngfilterdecode(size_t rowbytes, u8 pixel_depth, u8 *image, int rows, u8 *filters) {
	if (rows == 0) return;
	u8 *firstRow = malloc(rowbytes);
	memset(firstRow, 0, rowbytes);
	png_read_filter_row(rowbytes, pixel_depth, image, firstRow, filters[0]);
	free(firstRow);
	for (int i = 1; i < rows; i++) {
		png_read_filter_row(rowbytes, pixel_depth, image + (rowbytes * i), image + (rowbytes * (i - 1)), filters[i]);
	}
}

unsigned long fillinflatebuffer(void *buffer, size_t length) {
	void *randomBuffer = NULL;
	size_t lastBufferSize = 0, nextBufferSize = length/2;
	unsigned long destlen, sourcelen;
	int inflateResult;
	do {
		randomBuffer = growrandbuffer(randomBuffer, lastBufferSize, nextBufferSize);
		destlen = length;
		sourcelen = nextBufferSize;
		inflateResult = puff(buffer, &destlen, randomBuffer, &sourcelen);
		lastBufferSize = nextBufferSize;
		nextBufferSize *= 2;
	} while (inflateResult == 2);
	free(randomBuffer);
	return sourcelen;
}

void fillfilterbuffer(u8 *buffer, size_t length) {
	for (size_t i = 0; i < length; i++) {
		buffer[i] = randombytebelow(5);
	}
}
