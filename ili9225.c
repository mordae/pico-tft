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

#include <tft.h>

#if TFT_SWAP_XY
#define WIDTH 220
#define HEIGHT 176
#else
#define WIDTH 176
#define HEIGHT 220
#endif

const int tft_width = WIDTH / TFT_SCALE;
const int tft_height = HEIGHT / TFT_SCALE;

inline static void set_register(uint8_t reg, uint16_t value)
{
	uint8_t buf[2] = { value >> 8, value };
	tft_control(reg, buf, 2);
}

void tft_preflight(void)
{
	/* Reset power options. */
	set_register(0x10, 0x0000); // Set SAP, DSTB, STB
	set_register(0x11, 0x0000); // Set APON, PON, AON, VCI1EN, VC
	set_register(0x12, 0x0000); // Set BT, DC1, DC2, DC3
	set_register(0x13, 0x0000); // Set GVDD
	set_register(0x14, 0x0000); // Set VCOMH/VCOML voltage
	sleep_ms(40);

	/* Set power options. */
	set_register(0x11, 0x0018); // Set APON, PON, AON, VCI1EN, VC
	set_register(0x12, 0x6121); // Set BT, DC1, DC2, DC3
	set_register(0x13, 0x006f); // Set GVDD
	set_register(0x14, 0x495f); // Set VCOMH/VCOML voltage
	set_register(0x10, 0x0800); // Set SAP, DSTB, STB
	sleep_ms(10);

	set_register(0x11, 0x103b); // Set APON, PON, AON, VCI1EN, VC
	sleep_ms(50);

	/* Configure most of the display. */
	set_register(0x01, 0x001c); // 220 lines
	set_register(0x02, 0x0100); // INV0=1 (3 field interlace)

	uint16_t orient = (TFT_SWAP_XY << 3) | (TFT_FLIP_Y << 4) | ((!TFT_FLIP_X) << 5);
	set_register(0x03, 0x1000 | orient); // BGR=1 + orient

	set_register(0x07, 0x0000); // Display off
	set_register(0x08, 0x0808); // Back and front porch
	set_register(0x0b, 0x1100); // NO=1, STD=1, RTN=0 (16 clocks per line)
	set_register(0x0c, 0x0000); // RIM=0 (18b), DM=0, RM=0
	set_register(0x0f, 0xe001); // FOSC=1110 (500KHz), OSC_EN=1
	set_register(0x15, 0x0020); // VCIR=010 (2 clocks)
	set_register(0x20, 0x0000); // AD_LO=0 (position GRAM AC=0)
	set_register(0x21, 0x0000); // AD_HI=0

	/* Scrolling and update area setup. */
	set_register(0x30, 0x0000);
	set_register(0x31, 0x00db);
	set_register(0x32, 0x0000);
	set_register(0x33, 0x0000);
	set_register(0x34, 0x00db);
	set_register(0x35, 0x0000);
	set_register(0x36, 0x00af);
	set_register(0x37, 0x0000);
	set_register(0x38, 0x00db);
	set_register(0x39, 0x0000);

	/* Adjust gamma curve. */
	set_register(0x50, 0x0000);
	set_register(0x51, 0x0808);
	set_register(0x52, 0x080a);
	set_register(0x53, 0x000a);
	set_register(0x54, 0x0a08);
	set_register(0x55, 0x0808);
	set_register(0x56, 0x0000);
	set_register(0x57, 0x0a00);
	set_register(0x58, 0x0710);
	set_register(0x59, 0x0710);

	set_register(0x07, 0x0012);
	sleep_ms(50);

	set_register(0x07, 0x1017); // TEMON=1, GON=1, CL=1, REV=1, D=11

	/* 8.2.18 RAM Address Set */
	int pw = TFT_SWAP_XY ? HEIGHT : WIDTH;
	int ph = TFT_SWAP_XY ? WIDTH : HEIGHT;

	set_register(0x20, TFT_FLIP_Y ? 0 : pw - 1);
	set_register(0x21, TFT_FLIP_X ? ph - 1 : 0);
}

void tft_begin_sync(void)
{
	/* Activate GRAM write register. */
	tft_control(0x22, NULL, 0);
}
