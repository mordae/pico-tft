#include <pico/stdlib.h>

#include <hardware/spi.h>
#include <hardware/dma.h>
#include <hardware/pio.h>

#include <tft.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#if !defined(TFT_RST_DELAY)
#define TFT_RST_DELAY 150
#endif

#if !defined(TFT_HW_ACCEL)
#define TFT_HW_ACCEL 0
#endif

#if !defined(TFT_PIO)
#define TFT_PIO pio0
#endif

#if !defined(__weak)
#define __weak __attribute__((__weak__))
#endif

color_t tft_buffers[2][TFT_RAW_HEIGHT][TFT_RAW_WIDTH];
color_t (*tft_input)[TFT_RAW_WIDTH] = tft_buffers[0];
color_t (*tft_active)[TFT_RAW_WIDTH] = tft_buffers[1];

extern const color_t *tft_dma_script_0[TFT_RAW_HEIGHT * 2 + 2];
extern const color_t *tft_dma_script_1[TFT_RAW_HEIGHT * 2 + 2];

/* Clipping rectangle. */
int tft_clip_x0 = 0;
int tft_clip_y0 = 0;
int tft_clip_x1 = TFT_WIDTH;
int tft_clip_y1 = TFT_HEIGHT;

/* Origin for relative drawing. */
int tft_origin_x = 0;
int tft_origin_y = 0;

/* Font data. */
extern const uint8_t tft_font[256 * 16];

/* PIO State Machine used for scaling. */
static int pio_sm;

/* DMA channel to use for transmit. */
static int dma_ch_spi;

/* DMA channel to use for horizontal scaling. */
static int dma_ch_pio;

/* DMA channel to use for vertical scaling. */
static int dma_ch_ctrl;

/* Implemented by the driver. */
extern void tft_preflight(void);
extern void tft_begin_write(void);

inline static void select_register(void)
{
	gpio_put(TFT_RS_PIN, 0);
}

inline static void select_data(void)
{
	gpio_put(TFT_RS_PIN, 1);
}

inline static void select_chip(bool cs)
{
	gpio_put(TFT_CS_PIN, !cs);
}

__weak void tft_dma_channel_wait_for_finish_blocking(int dma_ch)
{
	dma_channel_wait_for_finish_blocking(dma_ch);
}

void tft_control(uint8_t reg, const uint8_t *bstr, int len)
{
	select_register();
	select_chip(true);
	spi_write_blocking(TFT_SPI_DEV, &reg, 1);

	select_data();

	for (int i = 0; i < len; i += 2) {
		uint16_t tmp = (bstr[i] << 8) | bstr[i + 1];
		spi_write16_blocking(TFT_SPI_DEV, &tmp, 1);
	}

	if (len & 1) {
		uint16_t tmp = bstr[len - 1] << 8;
		spi_write16_blocking(TFT_SPI_DEV, &tmp, 1);
	}

	select_chip(false);
}

#if defined(TFT_MISO_PIN)
void tft_read(uint8_t reg, uint8_t *bstr, int len, int delay_us)
{
	select_register();
	select_chip(true);
	spi_write_blocking(TFT_SPI_DEV, &reg, 1);

	if (delay_us >= 0)
		sleep_us(delay_us);

	select_data();

	const uint16_t blank = 0;
	uint16_t tmp;

	for (int i = 0; i < len; i += 2) {
		spi_read16_blocking(TFT_SPI_DEV, blank, &tmp, 1);
		bstr[i] = tmp >> 8;
		bstr[i + 1] = tmp;
	}

	if (len & 1) {
		spi_read16_blocking(TFT_SPI_DEV, blank, &tmp, 1);
		bstr[len - 1] = tmp >> 8;
	}

	select_chip(false);
}
#endif

void tft_init(void)
{
	unsigned rate = spi_init(TFT_SPI_DEV, TFT_BAUDRATE);
	spi_set_format(TFT_SPI_DEV, 16, 0, 0, SPI_MSB_FIRST);

	printf("tft: spi 16b rate=%u\n", rate);

#if defined(TFT_MISO_PIN)
	printf("tft: pins cs=%i, sck=%i, mosi=%i, miso=%i, rs=%i, rst=%i\n", TFT_CS_PIN,
	       TFT_SCK_PIN, TFT_MOSI_PIN, TFT_MISO_PIN, TFT_RS_PIN, TFT_RST_PIN);
#else
	printf("tft: pins cs=%i, sck=%i, mosi=%i, rs=%i, rst=%i\n", TFT_CS_PIN, TFT_SCK_PIN,
	       TFT_MOSI_PIN, TFT_RS_PIN, TFT_RST_PIN);
#endif

	//gpio_set_function(TFT_CS_PIN, GPIO_FUNC_SPI);
	gpio_init(TFT_CS_PIN);
	gpio_set_dir(TFT_CS_PIN, GPIO_OUT);
	gpio_put(TFT_CS_PIN, 1);

	gpio_set_function(TFT_SCK_PIN, GPIO_FUNC_SPI);
	gpio_set_function(TFT_MOSI_PIN, GPIO_FUNC_SPI);

#if defined(TFT_MISO_PIN)
	gpio_set_function(TFT_MISO_PIN, GPIO_FUNC_SPI);
#endif

	gpio_init(TFT_RST_PIN);
	gpio_set_dir(TFT_RST_PIN, GPIO_OUT);

	gpio_init(TFT_RS_PIN);
	gpio_set_dir(TFT_RS_PIN, GPIO_OUT);

	pio_sm = pio_claim_unused_sm(TFT_PIO, true);
	printf("tft: claimed pio%i.%i for horizontal scaling\n", TFT_PIO == pio0 ? 0 : 1, pio_sm);

	uint16_t pio_insns[] = {
		pio_encode_out(pio_y, 16),
		pio_encode_in(pio_y, 16),
		pio_encode_in(pio_y, 16),
	};

	pio_program_t pio_prog = {
		.instructions = pio_insns,
		.length = sizeof(pio_insns) / sizeof(*pio_insns),
		.origin = -1,
	};

	pio_prog.origin = pio_add_program(TFT_PIO, &pio_prog);

	printf("tft: pio program loaded at %i-%i\n", pio_prog.origin,
	       pio_prog.origin + pio_prog.length - 1);

	pio_sm_config sm_conf = pio_get_default_sm_config();
	sm_config_set_clkdiv_int_frac(&sm_conf, 1, 0);
	sm_config_set_in_shift(&sm_conf, false, true, 16);
	sm_config_set_out_shift(&sm_conf, false, true, 16);
	sm_config_set_wrap(&sm_conf, pio_prog.origin, pio_prog.origin + pio_prog.length - 1);

	pio_sm_restart(TFT_PIO, pio_sm);
	pio_sm_init(TFT_PIO, pio_sm, pio_prog.origin, &sm_conf);
	pio_sm_set_enabled(TFT_PIO, pio_sm, true);

	dma_ch_spi = dma_claim_unused_channel(true);
	printf("tft: claimed dma%i for spi\n", dma_ch_spi);

	dma_ch_pio = dma_claim_unused_channel(true);
	printf("tft: claimed dma%i for horizontal scaling\n", dma_ch_pio);

	dma_ch_ctrl = dma_claim_unused_channel(true);
	printf("tft: claimed dma%i for vertical scaling\n", dma_ch_ctrl);

	dma_channel_config dma_conf;

	dma_conf = dma_channel_get_default_config(dma_ch_ctrl);
	channel_config_set_transfer_data_size(&dma_conf, DMA_SIZE_32);
	channel_config_set_read_increment(&dma_conf, true);
	channel_config_set_write_increment(&dma_conf, false);
	channel_config_set_irq_quiet(&dma_conf, true);
	dma_channel_configure(dma_ch_ctrl, &dma_conf, &dma_hw->ch[dma_ch_pio].al3_read_addr_trig,
			      NULL, 1, false);

	dma_conf = dma_channel_get_default_config(dma_ch_pio);
	channel_config_set_transfer_data_size(&dma_conf, DMA_SIZE_16);
	channel_config_set_read_increment(&dma_conf, true);
	channel_config_set_write_increment(&dma_conf, false);
	channel_config_set_dreq(&dma_conf, pio_get_dreq(TFT_PIO, pio_sm, true));
	channel_config_set_chain_to(&dma_conf, dma_ch_ctrl);
	channel_config_set_irq_quiet(&dma_conf, true);
	dma_channel_configure(dma_ch_pio, &dma_conf, &TFT_PIO->txf[pio_sm], NULL, TFT_RAW_WIDTH,
			      false);

	dma_conf = dma_channel_get_default_config(dma_ch_spi);
	channel_config_set_transfer_data_size(&dma_conf, DMA_SIZE_16);
	channel_config_set_read_increment(&dma_conf, false);
	channel_config_set_write_increment(&dma_conf, false);
	channel_config_set_dreq(&dma_conf, spi_get_dreq(TFT_SPI_DEV, true));
	dma_channel_configure(dma_ch_spi, &dma_conf, &spi_get_hw(TFT_SPI_DEV)->dr,
			      &TFT_PIO->rxf[pio_sm], TFT_RAW_WIDTH * 2 * TFT_RAW_HEIGHT * 2, false);

	printf("tft: begin reset & preflight...\n");

	gpio_put(TFT_RST_PIN, 0);
	sleep_ms(10);

	gpio_put(TFT_RST_PIN, 1);
	sleep_ms(TFT_RST_DELAY);

	tft_preflight();
}

void tft_swap_buffers(void)
{
	void *tmp = tft_active;
	tft_active = tft_input;
	tft_input = tmp;
}

void tft_begin_sync(void)
{
	// Determine which script to use.
	if (tft_active == tft_buffers[0]) {
		dma_hw->ch[dma_ch_ctrl].al3_read_addr_trig = (uint32_t)tft_dma_script_0;
	} else if (tft_active == tft_buffers[1]) {
		dma_hw->ch[dma_ch_ctrl].al3_read_addr_trig = (uint32_t)tft_dma_script_1;
	} else {
		abort();
	}

	// Wait a bit for PIO FIFO to fill.
	sleep_us(1);

	// Wait for vblank.
	tft_begin_write();

	// Enable output to SPI.
	select_chip(true);
	dma_channel_start(dma_ch_spi);
}

void tft_wait_for_finish(void)
{
	while (true) {
		// Copy the latest script row address.
		unsigned read_addr = dma_hw->ch[dma_ch_ctrl].read_addr;

		// Determine where is the script pointing to.
		color_t *row = read_addr ? *(color_t **)read_addr : NULL;

		if (NULL == row) {
			// Script has finished.
			// Make sure to wait for the SPI DMA to finish.
			tft_dma_channel_wait_for_finish_blocking(dma_ch_spi);
			break;
		}

		// SPI is the channel that is going to be most busy.
		tft_dma_channel_wait_for_finish_blocking(dma_ch_spi);
	};

	// Make sure to wait for the peripheral itself.
	while (spi_is_busy(TFT_SPI_DEV))
		sleep_us(1);

	select_chip(false);
}

void tft_sync(void)
{
	tft_begin_sync();
	tft_wait_for_finish();
}

void tft_draw_rect(int x0, int y0, int x1, int y1, color_t color)
{
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

#if TFT_SWAP_XY
	for (int x = x0; x <= x1; x++)
		for (int y = y0; y <= y1; y++)
			tft_draw_pixel(x, y, color);
#else
	for (int y = y0; y <= y1; y++)
		for (int x = x0; x <= x1; x++)
			tft_draw_pixel(x, y, color);
#endif
}

void tft_fill(color_t color)
{
	for (int y = 0; y < TFT_RAW_HEIGHT; y++)
		for (int x = 0; x < TFT_RAW_WIDTH; x++)
			tft_input[y][x] = color;
}

void tft_draw_sprite(int x, int y, int w, int h, const color_t *data, uint16_t trsp)
{
	tft_draw_sprite_flipped(x, y, w, h, data, trsp, false, false, false);
}

void tft_draw_sprite_flipped(int x, int y, int w, int h, const color_t *data, uint16_t trsp,
			     bool flip_x, bool flip_y, bool swap_xy)
{
#define loop                                   \
	for (int sy = 0; sy < h; sy++)         \
		for (int sx = 0; sx < w; sx++) \
			if ((c = data[sy * w + sx]) != trsp)

	int c;
	int b = h - 1;
	int r = w - 1;

	if (flip_y && flip_x && swap_xy) {
		loop tft_draw_pixel(x + b - sy, y + r - sx, c);
	} else if (flip_y && flip_x && !swap_xy) {
		loop tft_draw_pixel(x + r - sx, y + b - sy, c);
	} else if (flip_y && !flip_x && swap_xy) {
		loop tft_draw_pixel(x + b - sy, y + sx, c);
	} else if (flip_y && !flip_x && !swap_xy) {
		loop tft_draw_pixel(x + sx, y + b - sy, c);
	} else if (!flip_y && flip_x && swap_xy) {
		loop tft_draw_pixel(x + sy, y + r - sx, c);
	} else if (!flip_y && flip_x && !swap_xy) {
		loop tft_draw_pixel(x + r - sx, y + sy, c);
	} else if (!flip_y && !flip_x && swap_xy) {
		loop tft_draw_pixel(x + sy, y + sx, c);
	} else {
		loop tft_draw_pixel(x + sx, y + sy, c);
	}

#undef loop
}

void tft_draw_glyph(int x, int y, color_t color, char c)
{
	const uint8_t *glyph = tft_font + (size_t)c * 16;

	for (int gx = 0; gx < 8; gx++) {
		for (int gy = 0; gy < 16; gy++) {
			if ((glyph[gy] >> (7 - gx)) & 1) {
				tft_draw_pixel(x + gx, y + gy, color);
			}
		}
	}
}

void tft_draw_string(int x, int y, color_t color, const char *fmt, ...)
{
	char buf[64];

	va_list ap;
	va_start(ap, fmt);
	int len = vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);

	if (len < 0)
		return;

	for (int i = 0; i < len; i++)
		tft_draw_glyph(x + i * 8, y, color, buf[i]);
}

void tft_draw_string_right(int x, int y, color_t color, const char *fmt, ...)
{
	char buf[64];

	va_list ap;
	va_start(ap, fmt);
	int len = vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);

	if (len < 0)
		return;

	x -= len * 8;

	for (int i = 0; i < len; i++)
		tft_draw_glyph(x + i * 8, y, color, buf[i]);
}

void tft_draw_string_center(int x, int y, color_t color, const char *fmt, ...)
{
	char buf[64];

	va_list ap;
	va_start(ap, fmt);
	int len = vsnprintf(buf, sizeof buf, fmt, ap);
	va_end(ap);

	if (len < 0)
		return;

	x -= len * 4;

	for (int i = 0; i < len; i++)
		tft_draw_glyph(x + i * 8, y, color, buf[i]);
}
