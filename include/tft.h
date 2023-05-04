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

#include <pico/stdlib.h>


/* TFT dimensions, driver-specific. */
extern const int tft_width;
extern const int tft_height;


/*
 * Color palette. We save only 4 bits per pixel instead of full 16.
 *
 * Default colors are:
 *
 *   0 - black
 *   1 - dark gray
 *   2 - light gray
 *   3 - white,
 *   4 - red
 *   5 - green
 *   6 - blue
 *   7 - light red
 *   8 - light green
 *   9 - light blue
 *  10 - yellow
 *  11 - cyan
 *  12 - purple
 *  13 - dark yellow
 *  14 - dark cyan
 *  15 - dark purple
 *
 * You should define some aliases and perhaps change the colors to
 * your liking. You can use tft_rgb() to generate the colors.
 */
extern uint16_t tft_palette[16];

/*
 * Generate 16bit color from the customary 3x 8bit RGB.
 */
inline static uint16_t tft_rgb(int r, int g, int b)
{
	r = (r >> 3) & 0b00011111;
	g = (g >> 2) & 0b00111111;
	b = (b >> 3) & 0b00011111;
	return (r << 11) | (g << 5) | b;
}


/*
 * Modified latin 16x8 bitmap font.
 * Populate it with tft_load_font().
 */
extern uint8_t tft_font[256 * 16];


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
 * Helper function to clamp values.
 */
inline static int tft_clamp(int x, int min, int max)
{
	if (x < min)
		return min;

	if (x > max)
		return max;

	return x;
}


/*
 * Color a single pixel.
 */
inline static void tft_draw_pixel(int x, int y, int color)
{
	x = tft_clamp(x, 0, tft_width - 1);
	y = tft_clamp(y, 0, tft_height - 1);
	color = tft_clamp(color, 0, 0x0f);

	uint8_t twopix = tft_input[(y * tft_width + x) >> 1];

	if (x & 1)
		twopix = (twopix & 0b11110000) | ((color & 0b1111) << 0);
	else
		twopix = (twopix & 0b00001111) | ((color & 0b1111) << 4);

	tft_input[(y * tft_width + x) >> 1] = twopix;
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
 * The coordinates indicate bottom left of the string.
 */
void tft_draw_string(int x, int y, int color, const char *str);


/*
 * Write directly into a control register.
 */
void tft_control(uint8_t reg, uint8_t *bstr, size_t len);
