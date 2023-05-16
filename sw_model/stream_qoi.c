#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "qoi.h"

static const unsigned char qoi_padding[8] = {0,0,0,0,0,0,0,1};

static qoi_rgba_t index[64];

static void qoi_write_32(unsigned char *bytes, uint32_t *p, uint32_t v)
{
	bytes[(*p)++] = (0xff000000 & v) >> 24;
	bytes[(*p)++] = (0x00ff0000 & v) >> 16;
	bytes[(*p)++] = (0x0000ff00 & v) >> 8;
	bytes[(*p)++] = (0x000000ff & v);
}

static int run;
static qoi_rgba_t px, px_prev;
static int px_end;


void s_qoi_encode_px(qoi_rgba_t px, int px_pos, uint8_t* px_enc, uint8_t* px_enc_len)
{
	uint8_t p = 0;

	if (px.v == px_prev.v) {
		run++;
		if (run == 62 || px_pos == px_end) {
			px_enc[p++] = QOI_OP_RUN | (run - 1);
			run = 0;
		}
	}
	else {
		int index_pos;

		if (run > 0) {
			px_enc[p++] = QOI_OP_RUN | (run - 1);
			run = 0;
		}

		index_pos = QOI_COLOR_HASH(px) % 64;

		if (index[index_pos].v == px.v) {
			px_enc[p++] = QOI_OP_INDEX | index_pos;
		}
		else {
			index[index_pos] = px;

			if (px.rgba.a == px_prev.rgba.a) {
				signed char vr = px.rgba.r - px_prev.rgba.r;
				signed char vg = px.rgba.g - px_prev.rgba.g;
				signed char vb = px.rgba.b - px_prev.rgba.b;

				signed char vg_r = vr - vg;
				signed char vg_b = vb - vg;

				if (
					vr > -3 && vr < 2 &&
					vg > -3 && vg < 2 &&
					vb > -3 && vb < 2
				) {
					px_enc[p++] = QOI_OP_DIFF | (vr + 2) << 4 | (vg + 2) << 2 | (vb + 2);
				}
				else if (
					vg_r >  -9 && vg_r <  8 &&
					vg   > -33 && vg   < 32 &&
					vg_b >  -9 && vg_b <  8
				) {
					px_enc[p++] = QOI_OP_LUMA     | (vg   + 32);
					px_enc[p++] = (vg_r + 8) << 4 | (vg_b +  8);
				}
				else {
					px_enc[p++] = QOI_OP_RGB;
					px_enc[p++] = px.rgba.r;
					px_enc[p++] = px.rgba.g;
					px_enc[p++] = px.rgba.b;
				}
			}
			else {
				px_enc[p++] = QOI_OP_RGBA;
				px_enc[p++] = px.rgba.r;
				px_enc[p++] = px.rgba.g;
				px_enc[p++] = px.rgba.b;
				px_enc[p++] = px.rgba.a;
			}
		}
	}
	px_prev = px;
	*px_enc_len = p;
}

void *s_qoi_encode(const void *data, const qoi_desc *desc, int *out_len)
{

	uint32_t i, max_size, p;
	int px_len, px_pos, channels;
	unsigned char* bytes;
	const unsigned char *pixels;

	uint8_t px_enc[5];
	uint8_t px_enc_len;

	if (
		data == 0 || out_len == 0 || desc == 0 ||
		desc->width == 0 || desc->height == 0 ||
		desc->channels < 3 || desc-> channels > 4 ||
		desc->colorspace > 1 ||
		desc->height >= QOI_PIXELS_MAX / desc->width
	) {
		return 0;
	}

	max_size =
		desc->width * desc->height * (desc->channels + 1) +
		QOI_HEADER_SIZE + sizeof(qoi_padding);
	
	p = 0;
	bytes = (unsigned char*) malloc(max_size);
	if (!bytes) {
		return 0;
	}

	qoi_write_32(bytes, &p, QOI_MAGIC);
	qoi_write_32(bytes, &p, desc->width);
	qoi_write_32(bytes, &p, desc->height);
	bytes[p++] = desc->channels;
	bytes[p++] = desc->colorspace;

	pixels = (const uint8_t *)data;

	QOI_ZEROARR(index);

	run = 0;
	px_prev.rgba.r = 0;
	px_prev.rgba.g = 0;
	px_prev.rgba.b = 0;
	px_prev.rgba.a = 255;
	px = px_prev;

	px_len = desc->width * desc->height * desc->channels;
	px_end = px_len - desc->channels;
	channels = desc->channels;

	for (px_pos = 0; px_pos < px_len; px_pos += channels) {
		px.rgba.r = pixels[px_pos + 0];
		px.rgba.g = pixels[px_pos + 1];
		px.rgba.b = pixels[px_pos + 2];

		if (channels == 4) {
			px.rgba.a = pixels[px_pos + 3];
		}

		s_qoi_encode_px(px, px_pos, px_enc, &px_enc_len);

		for (i = 0; i < px_enc_len; i++) {
			bytes[p++] = px_enc[i];
		}
	}

}


int s_qoi_write(const char* filename, const void* data, const qoi_desc *desc)
{
	FILE *f = fopen(filename, "wb");
	int size;
	void* encoded;

	printf("QOI write\n");
	if (!f) {
		return 0;
	}

	encoded = s_qoi_encode(data, desc, &size);
	if (!encoded) {
		fclose(f);
		return 0;
	}

	fwrite(encoded, 1, size, f);
	printf("size: %d\n", size);
	fclose(f);

	free(encoded);
	return size;
}

