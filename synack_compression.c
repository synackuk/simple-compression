#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#include "image.h"

#define MAGIC 'SUKC'

typedef struct {
	uint32_t magic; // 'SUKC'.. lol
	uint32_t width; // image width
	uint32_t height; // image height
} synackuk_compression_header_t;

typedef struct {
	uint8_t r; // red pixel value
	uint8_t g; // blue pixel value
	uint8_t b; // green pixel value
	uint8_t repeat; // the number of times this pixel is repeated
} pixel_t;

int compare_pixel(pixel_t* p1, pixel_t* p2) {
	return p1->r == p2->r && p1->g == p2->g && p1->b == p2->b;
}

void compress_data(uint32_t* image_in, uint32_t image_width, uint32_t image_height, char** image_out, size_t* image_size) {

	/* First build an image header */
	synackuk_compression_header_t header = (synackuk_compression_header_t){.magic = MAGIC, .width=image_width, .height=image_height};

	/* Then we build our output buffer */
	char* buf = malloc(image_width * image_height + sizeof(header));

	/* Copy the header to the start of our buffer */
	memcpy(buf, &header, sizeof(header));


	uint32_t* image_start = (uint32_t*)(buf + sizeof(header));

	uint32_t total_repeats = 0;
	uint32_t output_ptr = 0;

	for(uint32_t i = 0; i < image_width * image_height; i += 1) {
		pixel_t* pixel = (pixel_t*)&image_in[i];
		/* Either we use 0xff, but if that would exceed the image boundaries we use whatever the end of the buffer is */
		uint8_t max_repeat = ((image_width * image_height) - i) > 0xff ? 0xff : ((image_width * image_height) - i);

		uint8_t repeat = 0;
		for(uint8_t j = 1; j < max_repeat; j += 1) {
			pixel_t* next_pixel = (pixel_t*)&image_in[i + j];
			if(compare_pixel(pixel, next_pixel)) {
				repeat += 1;
			}
			else {
				break;
			}
		}
		i += repeat;
		total_repeats += repeat;

		pixel_t* output_pix = (pixel_t*)&image_start[output_ptr];
		memcpy(output_pix, pixel, sizeof(pixel_t));

		output_pix->repeat = repeat + 1;
		output_ptr += 1;
	}

	size_t len = image_width * image_height + sizeof(header) - total_repeats;
	*image_out = buf;
	*image_size = len;
}

void decompress_data(uint32_t* image_in, char** image_out, size_t* image_size) {
	synackuk_compression_header_t* header = (synackuk_compression_header_t*)image_in;

	if(header->magic != MAGIC) {
		return;
	}

	size_t outbuf_len = header->width * header->height;
	

	char* outbuf = malloc(outbuf_len);

	char* start = ((char*)image_in) + sizeof(synackuk_compression_header_t);
	outbuf_len -= sizeof(synackuk_compression_header_t);

	uint32_t place_in_out = 0;
	for(int i = 0; i < outbuf_len; i += 4) {
		pixel_t* pixel = (pixel_t*)&start[i];
		for(int i = 0; i < pixel->repeat; i += 1) {
			pixel_t* outpixel = (pixel_t*)(outbuf + place_in_out);
			memcpy(outpixel, pixel, sizeof(pixel_t));
			outpixel->repeat = 0xFF;
			place_in_out += 4;
		}
	}
	*image_out = outbuf;
	*image_size = outbuf_len;
}


int main() {
	char* output_img;
	size_t len;
	compress_data((uint32_t*)tst_image, tst_image_width, tst_image_height, &output_img, &len);
	char* decompressed_img;
	size_t decompressed_len;
	decompress_data((uint32_t*)output_img, &decompressed_img, &decompressed_len);

	/* Verify */

	printf("memcmp: %d\n", memcmp(tst_image, decompressed_img, decompressed_len));
}