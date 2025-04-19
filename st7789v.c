#include <pico/stdlib.h>

#include <tft.h>
#include <stdio.h>

#define WIDTH 240
#define HEIGHT 320

void tft_preflight(void)
{
	/* SWRESET: Software Reset */
	tft_control(0x01, NULL, 0);
	sleep_ms(120);

	/* SLPOUT: Sleep Out */
	tft_control(0x11, NULL, 0);
	sleep_ms(120);

	/* RDDID: Read Display ID */
	uint8_t ids[4] = { 0, 0, 0, 0 };
	tft_read(0x04, ids, 4, 1);

	/* GAMSET: Gamma Set */
	uint8_t gamset[] = { 1 };
	tft_control(0x26, gamset, sizeof gamset);

	/* MADCTL: Memory Access Data Control */
	uint8_t madctl = (0 << 5) | (TFT_FLIP_X << 6) | (TFT_FLIP_Y << 7) |
			 (0 << 3) /* BGR swap */ | (TFT_FLIP_Y << 4) | (TFT_FLIP_X << 2);
	tft_control(0x36, &madctl, sizeof madctl);

	/* COLMOD: Interface Pixel Format */
	uint8_t colmod = 0x6d; /* 16 bits in 565 format */
	tft_control(0x3a, &colmod, sizeof colmod);

	/* DISPON: Display On */
	tft_control(0x29, NULL, 0);
	sleep_ms(120);

	/* CASET: Column Address Set */
	uint8_t caset[] = { 0, 0, WIDTH >> 8, WIDTH & 0xff };
	tft_control(0x2a, caset, sizeof caset);

	/* PASET: Page Address Set */
	uint8_t paset[] = { 0, 0, HEIGHT >> 8, HEIGHT & 0xff };
	tft_control(0x2b, paset, sizeof paset);

	/* FRCTRL2: Frame Rate Control in Normal Mode */
	uint8_t frctr2[] = { 8 }; // 78 fps
	tft_control(0xc6, frctr2, sizeof frctr2);
}

void tft_begin_write(void)
{
#if defined(TFT_MISO_PIN) && TFT_VSYNC
	unsigned scanline;

	for (int i = 0; i < HEIGHT * 10; i++) {
		uint8_t scl[3];
		tft_read(0x45, scl, sizeof scl, 3);
		scanline = ((unsigned)scl[1] << 2) | (scl[2] >> 6);

		if (scanline >= 4 && scanline < 32) {
			break;
		}
	}
#endif

	/* 8.2.22 RAMWR: Memory Write */
	tft_control(0x2c, NULL, 0);
}
