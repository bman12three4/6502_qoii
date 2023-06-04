#include "qoi.h"
#include "stream_qoi.h"

#define QOI_START  ((char)(1 << 7))
#define QOI_ENCODE ((char)(0 << 7))

#define QOI_READ_FLAG ((char)(1 << 0))
#define QOI_WRITE_FLAG ((char)(1 << 1))

unsigned char* qoi = (unsigned char*)0x9000;
unsigned char* img = (unsigned char*)0x8000;
unsigned char* accel = (unsigned char*)0xa000;

qoi_desc desc;

uint8_t status;

static void qoi_write_32(unsigned char *bytes, uint32_t *p, uint32_t v)
{
	bytes[(*p)++] = (0xff000000 & v) >> 24;
	bytes[(*p)++] = (0x00ff0000 & v) >> 16;
	bytes[(*p)++] = (0x0000ff00 & v) >> 8;
	bytes[(*p)++] = (0x000000ff & v);
}

int main(void)
{
	uint32_t size;
	uint32_t s, d;
	char i;

	size = 4096L;
	d = 0;
	s = 0;

	desc.width = 32;
	desc.height = 32;
	desc.channels = 4;
	desc.colorspace = QOI_SRGB;

	qoi_write_32(qoi, &d, QOI_MAGIC);
	qoi_write_32(qoi, &d, desc.width);
	qoi_write_32(qoi, &d, desc.height);
	qoi[d++] = desc.channels;
	qoi[d++] = desc.colorspace;

	for (i = 0; i < 4; i++) {
		accel[i+4] = size >> i*8;
	}

	accel[3] = QOI_START | QOI_ENCODE;

	while(s < 4096) {
		status = accel[3];
		if (status & QOI_READ_FLAG) {
			accel[0] = img[s++];
		} else if (status & QOI_WRITE_FLAG) {
			qoi[d++] = accel[0];
		}
	}

	return 0;
}
