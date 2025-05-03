#pragma once
#include <stdint.h>
#include <stdbool.h>

#if !defined(__unused)
#define __unused __attribute__((__unused__))
#endif

#if !defined(TFT_VSYNC)
#define TFT_VSYNC 0
#endif

#if !defined(TFT_SWAP_XY)
#define TFT_SWAP_XY 0
#endif

#define TFT_RAW_WIDTH 120
#define TFT_RAW_HEIGHT 160

#if TFT_SWAP_XY
#define TFT_WIDTH (int)(TFT_RAW_HEIGHT)
#define TFT_HEIGHT (int)(TFT_RAW_WIDTH)
#else
#define TFT_WIDTH TFT_RAW_WIDTH
#define TFT_HEIGHT TFT_RAW_HEIGHT
#endif

#define TFT_RIGHT (TFT_WIDTH - 1)
#define TFT_BOTTOM (TFT_HEIGHT - 1)

/* Type alias for out 16b colors. */
typedef uint16_t color_t;

#define rgb565(r, g, b) ((((r) & 31) << 11) | (((g) & 63) << 5) | ((b) & 31))

#define rgb565_just_red(x) ((x) & 0xf800)
#define rgb565_just_green(x) ((x) & 0x07e0)
#define rgb565_just_blue(x) ((x) & 0x1f)

#define rgb565_red(x) ((rgb565_just_red((x)) >> 8) | (rgb565_just_red((x)) >> 13))
#define rgb565_green(x) ((rgb565_just_green((x)) >> 3) | (rgb565_just_green((x)) >> 9))
#define rgb565_blue(x) ((rgb565_just_blue((x)) << 3) | (rgb565_just_blue((x)) >> 2))

#define rgb_to_rgb565(r, g, b) ((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3))

inline static __unused color_t hsv_to_rgb565(float h, float s, float v)
{
	float r, g, b;

	h *= 6.0f;
	int i = (int)h;
	float f = h - i;
	float p = v * (1.0f - s);
	float q = v * (1.0f - s * f);
	float t = v * (1.0f - s * (1.0f - f));

	switch (i % 6) {
	case 0:
		r = v;
		g = t;
		b = p;
		break;
	case 1:
		r = q;
		g = v;
		b = p;
		break;
	case 2:
		r = p;
		g = v;
		b = t;
		break;
	case 3:
		r = p;
		g = q;
		b = v;
		break;
	case 4:
		r = t;
		g = p;
		b = v;
		break;
	default:
		r = v;
		g = p;
		b = q;
		break;
	}

	// Convert from 0-1 to 0-255 range
	uint8_t r8 = (uint8_t)(r * 255.0f);
	uint8_t g8 = (uint8_t)(g * 255.0f);
	uint8_t b8 = (uint8_t)(b * 255.0f);

	return rgb_to_rgb565(r8, g8, b8);
}

/*
 * Modified latin 16x8 bitmap font.
 * Populate it with tft_load_font().
 */
extern const uint8_t tft_font[256 * 16];

/* Initialize the screen. */
void tft_init(void);

/* Swap buffers. */
void tft_swap_buffers(void);

/* Start background sync of the back buffer. */
void tft_begin_sync(void);

/* Wait for the running sync to finish. */
void tft_wait_for_finish(void);

/* Begin sync and wait for it to finish. */
void tft_sync(void);

/* We are using double buffering. */
extern color_t tft_buffers[2][TFT_RAW_HEIGHT][TFT_RAW_WIDTH];

/* Current front-buffer (the one we paint into). */
extern color_t (*tft_input)[TFT_RAW_WIDTH];

/* Current back-buffer (the one being copied to screen). */
extern color_t (*tft_active)[TFT_RAW_WIDTH];

/* Clipping rectangle. */
extern int tft_clip_x0;
extern int tft_clip_y0;
extern int tft_clip_x1;
extern int tft_clip_y1;

/*
 * Set area to restrict painting to.
 * Unlike with tft_draw_rect, the x1, y1 bounds are non-inclusive.
 * Clipping applies to screen coordinates and ignores origin.
 */
inline static void __unused tft_set_clip(int x0, int y0, int x1, int y1)
{
	tft_clip_x0 = x0 < 0 ? 0 : x0;
	tft_clip_y0 = y0 < 0 ? 0 : y0;
	tft_clip_x1 = x1 > TFT_WIDTH ? TFT_WIDTH : x1;
	tft_clip_y1 = y1 > TFT_HEIGHT ? TFT_HEIGHT : y1;
}

/* Clear clipping to include whole display. */
inline static void __unused tft_clear_clip(void)
{
	tft_clip_x0 = 0;
	tft_clip_y0 = 0;
	tft_clip_x1 = TFT_WIDTH;
	tft_clip_y1 = TFT_HEIGHT;
}

/* Color a single pixel. */
inline static void __unused tft_draw_pixel_absolute(int x, int y, color_t color)
{
	if ((x < tft_clip_x0) || (x >= tft_clip_x1))
		return;

	if ((y < tft_clip_y0) || (y >= tft_clip_y1))
		return;

	if (x >= TFT_WIDTH || y >= TFT_HEIGHT)
		return;

#if TFT_SWAP_XY
	tft_input[x][y] = color;
#else
	tft_input[y][x] = color;
#endif
}

/* Origin for relative drawing. */
extern int tft_origin_x;
extern int tft_origin_y;

/* Set origin for relative drawing. */
inline static void __unused tft_set_origin(int x, int y)
{
	tft_origin_x = x;
	tft_origin_y = y;
}

/* Color a single pixel, relative to origin. */
inline static void __unused tft_draw_pixel(int x, int y, color_t color)
{
	x -= tft_origin_x;
	y -= tft_origin_y;
	tft_draw_pixel_absolute(x, y, color);
}

/*
 * Color a whole rect of pixels.
 */
void tft_draw_rect(int x0, int y0, int x1, int y1, color_t color);

/*
 * Paint the whole screen with a single color.
 */
void tft_fill(color_t color);

/*
 * Draw a simple sprite, starting from top-left coordinates.
 * Transparency indicates what color to treat as transparent.
 */
void tft_draw_sprite(int x, int y, int w, int h, const color_t *data, uint16_t trsp);

/*
 * Draw a flipped sprite, starting from top-left coordinates.
 * Transparency indicates what color to treat as transparent.
 * Combination of flips and swap allows for 90° rotations.
 */
void tft_draw_sprite_flipped(int x, int y, int w, int h, const color_t *data, uint16_t trsp,
			     bool flip_x, bool flip_y, bool swap_xy);

/*
 * Draw given glyph at specified coordinates.
 * The coordinates indicate bottom left of the glyph.
 */
void tft_draw_glyph(int x, int y, color_t color, char c);

/*
 * Draw given string at specified coordinates.
 *
 * The coordinates indicate bottom left of the string for the tft_draw_string,
 * bottom right for the tft_draw_string_right and bottom center for the
 * tft_draw_string_center.
 */
void tft_draw_string(int x, int y, color_t color, const char *fmt, ...)
	__attribute__((format(printf, 4, 5)));
void tft_draw_string_right(int x, int y, color_t color, const char *fmt, ...)
	__attribute__((format(printf, 4, 5)));
void tft_draw_string_center(int x, int y, color_t color, const char *fmt, ...)
	__attribute__((format(printf, 4, 5)));

/*
 * Write directly into a control register.
 */
void tft_control(uint8_t reg, const uint8_t *bstr, int len);

#if defined(TFT_MISO_PIN)
/*
 * Read directly from a control register.
 */
void tft_read(uint8_t reg, uint8_t *bstr, int len, int delay_us);
#endif

/*
 * Weak symbol that can be replaced with a version that yields to
 * other tasks while the DMA transfer is pending.
 */
void tft_dma_channel_wait_for_finish_blocking(int dma_ch);
