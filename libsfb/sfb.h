/******************************************************************************
 * Copyright (c) Jürgen Buchmüller <pullmoll@t-online.de>
 * All rights reserved.
 *
 * sfb.h - Simple Framebuffer Library interface
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 ******************************************************************************/

#include <sys/types.h>

struct sfb_s;
struct gdImageStruct;
typedef struct gdImageStruct* gdImagePtr;

#define	DEFAULT_FBDEV  "/dev/fb1"

/**
 * @brief Shift direction for @ref fb_shift()
 */
typedef enum {
    shift_left,
    shift_right,
    shift_up,
    shift_down
}   shift_dir_e;

typedef unsigned color_t;

#define RGB(r,g,b) (((color_t)r) << 16) | (((color_t)g) << 8) | (((color_t)b) << 0)

typedef enum {
    color_Alice_Blue           = RGB(0xF0,0xF8,0xFF),
    color_Antique_White        = RGB(0xFA,0xEB,0xD7),
    color_Aqua                 = RGB(0x00,0xFF,0xFF),
    color_Aquamarine           = RGB(0x7F,0xFF,0xD4),
    color_Azure                = RGB(0xF0,0xFF,0xFF),
    color_Beige                = RGB(0xF5,0xF5,0xDC),
    color_Bisque               = RGB(0xFF,0xE4,0xC4),
    color_Black                = RGB(0x00,0x00,0x00),
    color_Blanched_Almond      = RGB(0xFF,0xEB,0xCD),
    color_Blue                 = RGB(0x00,0x00,0xFF),
    color_Blue_Violet          = RGB(0x8A,0x2B,0xE2),
    color_Brown                = RGB(0xA5,0x2A,0x2A),
    color_Burlywood            = RGB(0xDE,0xB8,0x87),
    color_Cadet_Blue           = RGB(0x5F,0x9E,0xA0),
    color_Chartreuse           = RGB(0x7F,0xFF,0x00),
    color_Chocolate            = RGB(0xD2,0x69,0x1E),
    color_Coral                = RGB(0xFF,0x7F,0x50),
    color_Cornflower_Blue      = RGB(0x64,0x95,0xED),
    color_Cornsilk             = RGB(0xFF,0xF8,0xDC),
    color_Crimson              = RGB(0xDC,0x14,0x3C),
    color_Cyan                 = RGB(0x00,0xFF,0xFF),
    color_Dark_Blue            = RGB(0x00,0x00,0x8B),
    color_Dark_Cyan            = RGB(0x00,0x8B,0x8B),
    color_Dark_Goldenrod       = RGB(0xB8,0x86,0x0B),
    color_Dark_Gray            = RGB(0xA9,0xA9,0xA9),
    color_Dark_Green           = RGB(0x00,0x64,0x00),
    color_Dark_Khaki           = RGB(0xBD,0xB7,0x6B),
    color_Dark_Magenta         = RGB(0x8B,0x00,0x8B),
    color_Dark_Olive_Green     = RGB(0x55,0x6B,0x2F),
    color_Dark_Orange          = RGB(0xFF,0x8C,0x00),
    color_Dark_Orchid          = RGB(0x99,0x32,0xCC),
    color_Dark_Red             = RGB(0x8B,0x00,0x00),
    color_Dark_Salmon          = RGB(0xE9,0x96,0x7A),
    color_Dark_Sea_Green       = RGB(0x8F,0xBC,0x8F),
    color_Dark_Slate_Blue      = RGB(0x48,0x3D,0x8B),
    color_Dark_Slate_Gray      = RGB(0x2F,0x4F,0x4F),
    color_Dark_Turquoise       = RGB(0x00,0xCE,0xD1),
    color_Dark_Violet          = RGB(0x94,0x00,0xD3),
    color_Deep_Pink            = RGB(0xFF,0x14,0x93),
    color_Deep_Sky_Blue        = RGB(0x00,0xBF,0xFF),
    color_Dim_Gray             = RGB(0x69,0x69,0x69),
    color_Dodger_Blue          = RGB(0x1E,0x90,0xFF),
    color_Firebrick            = RGB(0xB2,0x22,0x22),
    color_Floral_White         = RGB(0xFF,0xFA,0xF0),
    color_Forest_Green         = RGB(0x22,0x8B,0x22),
    color_Fuchsia              = RGB(0xFF,0x01,0xFF),
    color_Gainsboro            = RGB(0xDC,0xDC,0xDC),
    color_Ghost_White          = RGB(0xF8,0xF8,0xFF),
    color_Gold                 = RGB(0xFF,0xD7,0x00),
    color_Goldenrod            = RGB(0xDA,0xA5,0x20),
    color_Gray                 = RGB(0xBE,0xBE,0xBE),
    color_Web_Gray             = RGB(0x80,0x80,0x80),
    color_Green                = RGB(0x00,0xFF,0x00),
    color_Web_Green            = RGB(0x00,0x80,0x00),
    color_Green_Yellow         = RGB(0xAD,0xFF,0x2F),
    color_Honeydew             = RGB(0xF0,0xFF,0xF0),
    color_Hot_Pink             = RGB(0xFF,0x69,0xB4),
    color_Indian_Red           = RGB(0xCD,0x5C,0x5C),
    color_Indigo               = RGB(0x4B,0x00,0x82),
    color_Ivory                = RGB(0xFF,0xFF,0xF0),
    color_Khaki                = RGB(0xF0,0xE6,0x8C),
    color_Lavender             = RGB(0xE6,0xE6,0xFA),
    color_Lavender_Blush       = RGB(0xFF,0xF0,0xF5),
    color_Lawn_Green           = RGB(0x7C,0xFC,0x00),
    color_Lemon_Chiffon        = RGB(0xFF,0xFA,0xCD),
    color_Light_Blue           = RGB(0xAD,0xD8,0xE6),
    color_Light_Coral          = RGB(0xF0,0x80,0x80),
    color_Light_Cyan           = RGB(0xE0,0xFF,0xFF),
    color_Light_Goldenrod      = RGB(0xFA,0xFA,0xD2),
    color_Light_Gray           = RGB(0xD3,0xD3,0xD3),
    color_Light_Green          = RGB(0x90,0xEE,0x90),
    color_Light_Pink           = RGB(0xFF,0xB6,0xC1),
    color_Light_Salmon         = RGB(0xFF,0xA0,0x7A),
    color_Light_Sea_Green      = RGB(0x20,0xB2,0xAA),
    color_Light_Sky_Blue       = RGB(0x87,0xCE,0xFA),
    color_Light_Slate_Gray     = RGB(0x77,0x88,0x99),
    color_Light_Steel_Blue     = RGB(0xB0,0xC4,0xDE),
    color_Light_Yellow         = RGB(0xFF,0xFF,0xE0),
    color_Lime                 = RGB(0x00,0xFF,0x01),
    color_Lime_Green           = RGB(0x32,0xCD,0x32),
    color_Linen                = RGB(0xFA,0xF0,0xE6),
    color_Magenta              = RGB(0xFF,0x00,0xFF),
    color_Maroon               = RGB(0xB0,0x30,0x60),
    color_Web_Maroon           = RGB(0x80,0x00,0x00),
    color_Medium_Aquamarine    = RGB(0x66,0xCD,0xAA),
    color_Medium_Blue          = RGB(0x00,0x00,0xCD),
    color_Medium_Orchid        = RGB(0xBA,0x55,0xD3),
    color_Medium_Purple        = RGB(0x93,0x70,0xDB),
    color_Medium_Sea_Green     = RGB(0x3C,0xB3,0x71),
    color_Medium_Slate_Blue    = RGB(0x7B,0x68,0xEE),
    color_Medium_Spring_Green  = RGB(0x00,0xFA,0x9A),
    color_Medium_Turquoise     = RGB(0x48,0xD1,0xCC),
    color_Medium_Violet_Red    = RGB(0xC7,0x15,0x85),
    color_Midnight_Blue        = RGB(0x19,0x19,0x70),
    color_Mint_Cream           = RGB(0xF5,0xFF,0xFA),
    color_Misty_Rose           = RGB(0xFF,0xE4,0xE1),
    color_Moccasin             = RGB(0xFF,0xE4,0xB5),
    color_Navajo_White         = RGB(0xFF,0xDE,0xAD),
    color_Navy_Blue            = RGB(0x00,0x00,0x80),
    color_Old_Lace             = RGB(0xFD,0xF5,0xE6),
    color_Olive                = RGB(0x80,0x80,0x00),
    color_Olive_Drab           = RGB(0x6B,0x8E,0x23),
    color_Orange               = RGB(0xFF,0xA5,0x00),
    color_Orange_Red           = RGB(0xFF,0x45,0x00),
    color_Orchid               = RGB(0xDA,0x70,0xD6),
    color_Pale_Goldenrod       = RGB(0xEE,0xE8,0xAA),
    color_Pale_Green           = RGB(0x98,0xFB,0x98),
    color_Pale_Turquoise       = RGB(0xAF,0xEE,0xEE),
    color_Pale_Violet_Red      = RGB(0xDB,0x70,0x93),
    color_Papaya_Whip          = RGB(0xFF,0xEF,0xD5),
    color_Peach_Puff           = RGB(0xFF,0xDA,0xB9),
    color_Peru                 = RGB(0xCD,0x85,0x3F),
    color_Pink                 = RGB(0xFF,0xC0,0xCB),
    color_Plum                 = RGB(0xDD,0xA0,0xDD),
    color_Powder_Blue          = RGB(0xB0,0xE0,0xE6),
    color_Purple               = RGB(0xA0,0x20,0xF0),
    color_Web_Purple           = RGB(0x80,0x00,0x80),
    color_Rebecca_Purple       = RGB(0x66,0x33,0x99),
    color_Red                  = RGB(0xFF,0x00,0x00),
    color_Rosy_Brown           = RGB(0xBC,0x8F,0x8F),
    color_Royal_Blue           = RGB(0x41,0x69,0xE1),
    color_Saddle_Brown         = RGB(0x8B,0x45,0x13),
    color_Salmon               = RGB(0xFA,0x80,0x72),
    color_Sandy_Brown          = RGB(0xF4,0xA4,0x60),
    color_Sea_Green            = RGB(0x2E,0x8B,0x57),
    color_Seashell             = RGB(0xFF,0xF5,0xEE),
    color_Sienna               = RGB(0xA0,0x52,0x2D),
    color_Silver               = RGB(0xC0,0xC0,0xC0),
    color_Sky_Blue             = RGB(0x87,0xCE,0xEB),
    color_Slate_Blue           = RGB(0x6A,0x5A,0xCD),
    color_Slate_Gray           = RGB(0x70,0x80,0x90),
    color_Snow                 = RGB(0xFF,0xFA,0xFA),
    color_Spring_Green         = RGB(0x00,0xFF,0x7F),
    color_Steel_Blue           = RGB(0x46,0x82,0xB4),
    color_Tan                  = RGB(0xD2,0xB4,0x8C),
    color_Teal                 = RGB(0x00,0x80,0x80),
    color_Thistle              = RGB(0xD8,0xBF,0xD8),
    color_Tomato               = RGB(0xFF,0x63,0x47),
    color_Turquoise            = RGB(0x40,0xE0,0xD0),
    color_Violet               = RGB(0xEE,0x82,0xEE),
    color_Wheat                = RGB(0xF5,0xDE,0xB3),
    color_White                = RGB(0xFF,0xFF,0xFF),
    color_White_Smoke          = RGB(0xF5,0xF5,0xF5),
    color_Yellow               = RGB(0xFF,0xFF,0x00),
    color_Yellow_Green         = RGB(0x9A,0xCD,0x32),
}   color_e;

extern int fb_init(struct sfb_s** sfb, const char* devname);
extern void fb_exit(struct sfb_s** sfb);

extern const char* fb_devname(struct sfb_s* sfb);
extern int fb_x(struct sfb_s* sfb);
extern int fb_y(struct sfb_s* sfb);
extern int fb_w(struct sfb_s* sfb);
extern int fb_h(struct sfb_s* sfb);
extern int fb_cx(struct sfb_s* sfb);
extern int fb_cy(struct sfb_s* sfb);
extern int fb_bpp(struct sfb_s* sfb);
extern void fb_gotoxy(struct sfb_s* sfb, int x, int y);

extern void fb_clear(struct sfb_s* sfb);
extern color_t fb_getpixel(struct sfb_s* sfb, int x, int y);
extern void fb_setpixel(struct sfb_s* sfb, int x, int y, color_t color);
extern void fb_hline(struct sfb_s* sfb, int x, int y, int l, color_t color);
extern void fb_vline(struct sfb_s* sfb, int x, int y, int l, color_t color);
extern void fb_line(struct sfb_s* sfb, int x1, int y1, int x2, int y2, color_t color);
extern void fb_rect(struct sfb_s* sfb, int x1, int y1, int x2, int y2, color_t color);
extern void fb_fill(struct sfb_s* sfb, int x1, int y1, int x2, int y2, color_t color);
extern void fb_circle_octants(struct sfb_s* sfb, unsigned char oct, int x, int y, int r, color_t color);
extern void fb_circle(struct sfb_s* sfb, int x, int y, int r, color_t color);
extern void fb_disc_octants(struct sfb_s* sfb, unsigned char oct, int x, int y, int r, color_t color);
extern void fb_disc(struct sfb_s* sfb, int x, int y, int r, color_t color);
extern void fb_shift(struct sfb_s* sfb, shift_dir_e dir, int pixels);
extern void fb_putc(struct sfb_s* sfb, color_t color, char ch);
extern void fb_puts(struct sfb_s* sfb, color_t color, const char* text);
extern size_t fb_printf(struct sfb_s* sfb, color_t color, const char* format, ...);
extern void fb_dump(struct sfb_s* sfb, gdImagePtr im);
