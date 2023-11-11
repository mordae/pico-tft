/*
 * Copyright Jan Hamal Dvořák <mordae@anilinux.org>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <pico/stdlib.h>
#include <pico/tft.h>
#include <hardware/spi.h>
#include <hardware/dma.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * We are using double buffering.
 *
 * One buffer is being written to by the client.
 * The other buffer is being transmitted.
 *
 * After every cycle the buffers are rotated.
 */
static uint8_t *buffer[2];

/*
 * Per-row transfer buffers. These are also two.
 * We are writing into one while the other is read by DMA.
 */
static uint16_t *txbuf[2];

/* Currently inactive buffer that is to be sent to the display. */
uint8_t *tft_committed;

/* Current active buffer that is to be written into. */
uint8_t *tft_input;

/* Font data. */
extern uint8_t tft_font[256 * 16];

/* Palette colors. */
uint16_t tft_palette[256] = {
	0x0000, 0x1082, 0x2104, 0x31a6, 0x4228, 0x52aa, 0x632c, 0x73ae, 0x8c51, 0x9cd3, 0xad55,
	0xbdd7, 0xce59, 0xdefb, 0xef7d, 0xffff, 0x2082, 0x20a2, 0x20e2, 0x2102, 0x1902, 0x1102,
	0x1102, 0x1103, 0x1104, 0x10c4, 0x10a4, 0x1084, 0x1884, 0x2084, 0x2083, 0x2083, 0x2041,
	0x2081, 0x20c1, 0x2101, 0x1101, 0x0901, 0x0902, 0x0903, 0x0904, 0x08c4, 0x0864, 0x0844,
	0x1044, 0x2044, 0x2043, 0x2042, 0x2000, 0x2060, 0x20c0, 0x1900, 0x1100, 0x0100, 0x0101,
	0x0102, 0x0104, 0x00a4, 0x0044, 0x0004, 0x1004, 0x1804, 0x2003, 0x2001, 0x4186, 0x41a6,
	0x41e6, 0x4206, 0x3a06, 0x3206, 0x3206, 0x3207, 0x3208, 0x31c8, 0x31a8, 0x3188, 0x3988,
	0x4188, 0x4187, 0x4187, 0x4104, 0x4164, 0x41c4, 0x3a04, 0x3204, 0x2204, 0x2205, 0x2206,
	0x2208, 0x21a8, 0x2148, 0x2108, 0x3108, 0x3908, 0x4107, 0x4105, 0x4082, 0x4102, 0x41a2,
	0x3a02, 0x2a02, 0x1a02, 0x1203, 0x1206, 0x1208, 0x1168, 0x10e8, 0x1888, 0x2888, 0x3888,
	0x4086, 0x4084, 0x4000, 0x40c0, 0x4180, 0x3a00, 0x2200, 0x0a00, 0x0202, 0x0205, 0x0208,
	0x0148, 0x0088, 0x0808, 0x2008, 0x3808, 0x4006, 0x4003, 0x830c, 0x836c, 0x83cc, 0x7c0c,
	0x740c, 0x640c, 0x640d, 0x640e, 0x6410, 0x63b0, 0x6350, 0x6310, 0x7310, 0x7b10, 0x830f,
	0x830d, 0x8208, 0x82c8, 0x8388, 0x7c08, 0x6408, 0x4c08, 0x440a, 0x440d, 0x4410, 0x4350,
	0x4290, 0x4a10, 0x6210, 0x7a10, 0x820e, 0x820b, 0x8104, 0x8224, 0x8344, 0x7404, 0x5404,
	0x2c04, 0x2407, 0x240b, 0x2410, 0x22f0, 0x21d0, 0x2910, 0x5110, 0x7110, 0x810d, 0x8108,
	0x8000, 0x8180, 0x8300, 0x7400, 0x4400, 0x1400, 0x0404, 0x040a, 0x0410, 0x0290, 0x0110,
	0x1010, 0x4010, 0x7010, 0x800c, 0x8006, 0xfdf7, 0xfeb7, 0xff77, 0xf7f7, 0xdff7, 0xc7f7,
	0xbff9, 0xbffc, 0xbfff, 0xbf3f, 0xbe7f, 0xc5ff, 0xddff, 0xf5ff, 0xfdfd, 0xfdfa, 0xfc10,
	0xfd70, 0xfef0, 0xeff0, 0xbff0, 0x8ff0, 0x87f3, 0x87f9, 0x87ff, 0x867f, 0x84ff, 0x8c1f,
	0xbc1f, 0xec1f, 0xfc1b, 0xfc15, 0xfa08, 0xfc28, 0xfe68, 0xe7e8, 0x9fe8, 0x5fe8, 0x47ee,
	0x47f6, 0x47ff, 0x45bf, 0x439f, 0x5a1f, 0x9a1f, 0xe21f, 0xfa19, 0xfa10, 0xf800, 0xfb00,
	0xfde0, 0xdfe0, 0x87e0, 0x27e0, 0x07e8, 0x07f3, 0x07ff, 0x04ff, 0x021f, 0x201f, 0x801f,
	0xd81f, 0xf817, 0xf80c,
};

/* DMA channel to use for transmit. */
static int dma_ch;
static dma_channel_config dma_conf;

/* Implemented by the driver. */
extern void tft_preflight(void);
extern void tft_begin_sync(void);

inline static void select_register(void)
{
	gpio_put(TFT_RS_PIN, 0);
}

inline static void select_data(void)
{
	gpio_put(TFT_RS_PIN, 1);
}

static void transmit_blocking(const void *buf, size_t len)
{
	dma_channel_wait_for_finish_blocking(dma_ch);
	size_t wr = spi_write_blocking(TFT_SPI_DEV, buf, len);

	if (wr < len)
		panic("tft: transmit_blocking: written < len");
}

static void transmit_dma(const void *buf, size_t len)
{
	dma_channel_wait_for_finish_blocking(dma_ch);
	dma_channel_configure(dma_ch, &dma_conf, &spi_get_hw(TFT_SPI_DEV)->dr, buf, len, true);
}

static void write_buffer_dma(void *bstr, size_t len)
{
	select_data();
	transmit_dma(bstr, len);
}

void tft_control(uint8_t reg, uint8_t *bstr, size_t len)
{
	select_register();
	transmit_blocking(&reg, 1);

	select_data();
	transmit_blocking(bstr, len);
}

void tft_init(void)
{
	int framebuf_len = tft_width * tft_height;
	int txbuf_len = tft_width * 2 * TFT_SCALE;

	printf("tft: Allocate buffers: %i, %i, %i, %i\n", framebuf_len, framebuf_len, txbuf_len,
	       txbuf_len);

	buffer[0] = malloc(framebuf_len);
	buffer[1] = malloc(framebuf_len);

	txbuf[0] = malloc(txbuf_len);
	txbuf[1] = malloc(txbuf_len);

	if ((!buffer[0]) || (!buffer[1]) || (!txbuf[0]) || (!txbuf[1]))
		panic("tft: tft_init: malloc failed");

	tft_input = buffer[0];
	tft_committed = buffer[1];

	printf("tft: Initialize default palette");

	unsigned rate = spi_init(TFT_SPI_DEV, TFT_BAUDRATE);
	printf("tft: Configured SPI: rate=%u\n", rate);

	printf("tft: Configure pins: cs=%i, sck=%i, mosi=%i, rs=%i, rst=%i\n", TFT_CS_PIN,
	       TFT_SCK_PIN, TFT_MOSI_PIN, TFT_RS_PIN, TFT_RST_PIN);

	gpio_set_function(TFT_CS_PIN, GPIO_FUNC_SPI);
	gpio_set_function(TFT_SCK_PIN, GPIO_FUNC_SPI);
	gpio_set_function(TFT_MOSI_PIN, GPIO_FUNC_SPI);

	gpio_init(TFT_RST_PIN);
	gpio_set_dir(TFT_RST_PIN, GPIO_OUT);

	gpio_init(TFT_RS_PIN);
	gpio_set_dir(TFT_RS_PIN, GPIO_OUT);

	dma_ch = dma_claim_unused_channel(true);
	printf("tft: Configured DMA channel: %i\n", dma_ch);

	dma_conf = dma_channel_get_default_config(dma_ch);
	channel_config_set_transfer_data_size(&dma_conf, DMA_SIZE_8);
	channel_config_set_dreq(&dma_conf, spi_get_dreq(TFT_SPI_DEV, true));

	printf("tft: Begin reset & preflight...\n");

	gpio_put(TFT_RST_PIN, 0);
	sleep_ms(50);

	gpio_put(TFT_RST_PIN, 1);
	sleep_ms(50);

	tft_preflight();

	printf("tft: Fill screen with black...\n");
	tft_fill(0);
	tft_swap_buffers();
	tft_sync();
}

void tft_swap_buffers(void)
{
	uint8_t *tmp;

	tmp = tft_committed;
	tft_committed = tft_input;
	tft_input = tmp;
}

void tft_sync(void)
{
	tft_begin_sync();

	for (int y = 0; y < tft_height; y++) {
		uint16_t *buf = txbuf[y & 1];
		uint16_t *bufptr = buf;

		for (int x = 0; x < tft_width; x++) {
			int i = y * tft_width + x;
			uint16_t color = tft_palette[tft_committed[i]];

			for (int i = 0; i < TFT_SCALE; i++)
				*bufptr++ = __builtin_bswap16(color);
		}

		for (int i = 0; i < TFT_SCALE; i++) {
			dma_channel_wait_for_finish_blocking(dma_ch);
			write_buffer_dma(buf, tft_width * 2 * TFT_SCALE);
		}
	}

	dma_channel_wait_for_finish_blocking(dma_ch);
}

void tft_swap_sync(void)
{
	tft_swap_buffers();
	tft_sync();
}

inline static int tft_clamp(int x, int min, int max)
{
	if (x < min)
		return min;

	if (x > max)
		return max;

	return x;
}

void tft_draw_rect(int x0, int y0, int x1, int y1, int color)
{
	x0 = tft_clamp(x0, 0, tft_width - 1);
	x1 = tft_clamp(x1, 0, tft_width - 1);
	y0 = tft_clamp(y0, 0, tft_height - 1);
	y1 = tft_clamp(y1, 0, tft_height - 1);
	color = tft_clamp(color, 0, 0x0f);

	if (x0 > x1) {
		int tmp = x0;
		x0 = x1;
		x1 = tmp;
	}

	if (y0 > y1) {
		int tmp = y0;
		y0 = y1;
		y1 = tmp;
	}

	for (int y = y0; y <= y1; y++) {
		for (int x = x0; x <= x1; x++) {
			tft_draw_pixel(x, y, color);
		}
	}
}

void tft_fill(int color)
{
	memset(tft_input, color, tft_width * tft_height);
}

void tft_draw_glyph(int x, int y, int color, char c)
{
	uint8_t *glyph = tft_font + (size_t)c * 16;

	for (int gx = 0; gx < 8; gx++) {
		for (int gy = 0; gy < 16; gy++) {
			if ((glyph[gy] >> (7 - gx)) & 1) {
				tft_draw_pixel(x + gx, y + gy, color);
			}
		}
	}
}

void tft_draw_string(int x, int y, int color, const char *str)
{
	int len = strlen(str);

	for (int i = 0; i < len; i++)
		tft_draw_glyph(x + i * 8, y, color, str[i]);
}

void tft_draw_string_right(int x, int y, int color, const char *str)
{
	int len = strlen(str);

	x -= len * 8;

	for (int i = 0; i < len; i++)
		tft_draw_glyph(x + i * 8, y, color, str[i]);
}

void tft_draw_string_center(int x, int y, int color, const char *str)
{
	int len = strlen(str);

	x -= len * 4;

	for (int i = 0; i < len; i++)
		tft_draw_glyph(x + i * 8, y, color, str[i]);
}
