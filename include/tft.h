/*
 * Copyright (C) 2022 Jan Hamal Dvořák <mordae@anilinux.org>
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

#pragma once
#define TFT_DRIVER_ST7735 1
#define TFT_DRIVER_ILI9225 2
#define TFT_DRIVER_ILI9341 3

#include <pico/stdlib.h>

#if !defined(TFT_SCALE)
#define TFT_SCALE 1
#elif (TFT_SCALE != 1) && (TFT_SCALE != 2) && (TFT_SCALE != 4)
#error TFT_SCALE must be 1, 2, or 4
#endif

#if !defined(TFT_SWAP_XY)
#define TFT_SWAP_XY 0
#endif

#if TFT_DRIVER == TFT_DRIVER_ST7735
#define TFT_RAW_WIDTH 128
#define TFT_RAW_HEIGHT 160
#elif TFT_DRIVER == TFT_DRIVER_ILI9225
#define TFT_RAW_WIDTH 176
#define TFT_RAW_HEIGHT 220
#elif TFT_DRIVER == TFT_DRIVER_ILI9341
#define TFT_RAW_WIDTH 240
#define TFT_RAW_HEIGHT 320
#endif

#if TFT_SWAP_XY
#define TFT_WIDTH (int)(TFT_RAW_HEIGHT / TFT_SCALE)
#define TFT_HEIGHT (int)(TFT_RAW_WIDTH / TFT_SCALE)
#else
#define TFT_WIDTH (int)(TFT_RAW_WIDTH / TFT_SCALE)
#define TFT_HEIGHT (int)(TFT_RAW_HEIGHT / TFT_SCALE)
#endif

/*
 * Modified latin 16x8 bitmap font.
 * Populate it with tft_load_font().
 */
extern uint8_t tft_font[256 * 16];

/*
 * Color palette mapping into RGB565.
 */
extern uint16_t tft_palette[256];

/*
 * Initialize the screen.
 */
void tft_init(void);

/*
 * Swap buffers to allow for a background sync.
 */
void tft_swap_buffers(void);

/*
 * Start display synchronization cycle.
 * Blocks until it's done so the you do not clobber the back buffer.
 */
void tft_sync(void);

/* Shortcut to swap buffers and sync at once. */
void tft_swap_sync(void);

/*
 * Here goes whatever should be on the screen.
 * First come the Y rows, then the X columns.
 */
extern uint8_t *tft_input;

/*
 * Color a single pixel.
 */
inline static void __unused tft_draw_pixel(int x, int y, int color)
{
	if ((x >= TFT_WIDTH) || (x < 0))
		return;

	if ((y >= TFT_HEIGHT) || (y < 0))
		return;

	int i = y * TFT_WIDTH + x;
	tft_input[i] = color;
}

/*
 * Color a whole rect of pixels.
 */
void tft_draw_rect(int x0, int y0, int x1, int y1, int color);

/*
 * Paint the whole screen with a single color.
 */
void tft_fill(int color);

/*
 * Draw given glyph at specified coordinates.
 * The coordinates indicate bottom left of the glyph.
 */
void tft_draw_glyph(int x, int y, int color, char c);

/*
 * Draw given string at specified coordinates.
 *
 * The coordinates indicate bottom left of the string for the tft_draw_string,
 * bottom right for the tft_draw_string_right and bottom center for the
 * tft_draw_string_center.
 */
void tft_draw_string(int x, int y, int color, const char *str);
void tft_draw_string_right(int x, int y, int color, const char *str);
void tft_draw_string_center(int x, int y, int color, const char *str);

/*
 * Write directly into a control register.
 */
void tft_control(uint8_t reg, const uint8_t *bstr, int len);

#if defined(TFT_MISO_PIN)
/*
 * Read directly from a control register.
 */
void tft_read(uint8_t reg, uint8_t *bstr, int len);
#endif

/*
 * Weak symbol that can be replaced with a version that yields to
 * other tasks while the DMA transfer is pending.
 */
void tft_dma_channel_wait_for_finish_blocking(int dma_ch);
