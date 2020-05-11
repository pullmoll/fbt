/******************************************************************************
 * Copyright (c) Jürgen Buchmüller <pullmoll@t-online.de>
 * All rights reserved.
 *
 * sfb.c - Simple Framebuffer Library implementation
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


#if defined(HAVE_CONFIG_H)
#include "config.h"
#endif

#if !defined(DEFAULT_FBDEV)
#define	DEFAULT_FBDEV  "/dev/fb1"
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#if defined(HAVE_STDINT_H)
#include <stdint.h>
#endif
#include <assert.h>

#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_SYS_IOCTL_H)
#include <sys/ioctl.h>
#endif
#if defined(HAVE_SYS_MMAN_H)
#include <sys/mman.h>
#endif
#if defined(HAVE_SYS_TYPES_H)
#include <sys/types.h>
#endif
#if defined(HAVE_SYS_STAT_H)
#include <sys/stat.h>
#endif
#if defined(HAVE_FCNTL_H)
#include <fcntl.h>
#endif
#if defined(HAVE_LINUX_FB_H)
#include <linux/fb.h>
#endif

#if defined(HAVE_GD_H)
#include <gd.h>
#endif

#include "font.h"
#include "sfb.h"

#if !defined(MIN)
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#if !defined(MAX)
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif

#if !defined(BOUND)
#define	BOUND(a,b,c) MIN(MAX(a,b),c)
#endif

#define	SFB_MAGIC   0x71a8402bu

typedef struct sfb_s {
    /** @brief magic value to check for invalid sfb_s* */
    uint32_t magic;

    /** @brief most recent error message */
    char errmsg[256];

    /** @brief frame buffer device name */
    const char* devname;

    /** @brief frame buffer file descripttor */
    int fd;

    /** @brief frame buffer x offset in pixels */
    int x;

    /** @brief frame buffer y offset in pixels */
    int y;

    /** @brief frame buffer width in pixels */
    int w;

    /** @brief frame buffer height in pixels */
    int h;

    /** @brief frame buffer depth (bits per pixel) */
    int bpp;

    /** @brief frame buffer size in bytes */
    size_t size;

    /** @brief frame buffer stride for scan lines */
    size_t stride;

    /** @brief frame buffer pointer to memory map */
    uint8_t *fbp;

    /** @brief pointer to the function to convert R, G, and B to a pixel value */
    color_t (*rgb2pix)(int r, int g, int b);

    /** @brief pointer to the function to get a pixel for a specific depth */
    uint32_t (*getpixel)(struct sfb_s* sfb, int x, int y);

    /** @brief pointer to the function to set a pixel for a specific depth */
    void (*setpixel)(struct sfb_s* sfb, int x, int y);

    /** @brief pointer to the function to write a horizontal line for a specific depth */
    void (*hline)(struct sfb_s* sfb, int x, int y, int l);

    /** @brief pointer to the function to write a vertical line for a specific depth */
    void (*vline)(struct sfb_s* sfb, int x, int y, int l);

    /** @brief pointer to font to use */
    const fbfont_t* font;

    /** @brief current background mode */
    color_t opaque;

    /** @brief current background color */
    color_t bgcolor;

    /** @brief current foreground color */
    color_t fgcolor;

    /** @brief cursor x coordinate */
    int cursor_x;

    /** @brief cursor y coordinate */
    int cursor_y;
}   sfb_t;

/**
 * @brief Check for a valid pointer to a struct sfb_s
 */
#define CHECK_FB(_fb) do {		    \
    if (!_fb || SFB_MAGIC != _fb->magic) {  \
	return;				    \
    }					    \
} while (0)
/**
 * @brief Check for a valid pointer to a struct sfb_s
 */
#define CHECK_FB_RET(_fb,_retval) do {	    \
    if (!_fb || SFB_MAGIC != _fb->magic) {  \
	return _retval;			    \
    }					    \
} while (0)

/**
 * @brief Print an error message into the frame buffer context's buffer
 * @param fb pointer to the frame buffer context
 * @param format C format string followed by optional parameters
 */
static void error(sfb_t* fb, const char* format, ...)
{
    CHECK_FB(fb);
    va_list ap;

    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);

    va_start(ap, format);
    vsnprintf(fb->errmsg, sizeof(fb->errmsg), format, ap);
    va_end(ap);
}

static void swap_fg_bg(sfb_t* fb)
{
    const uint32_t temp = fb->fgcolor;
    fb->fgcolor = fb->bgcolor;
    fb->bgcolor = temp;
}

/**
 * @brief Convert R, G, and B to monochrome
 * @param r red value (0 … 255)
 * @param g green value (0 … 255)
 * @param b blue value (0 … 255)
 * @return either black (0) or white (1)
 */
static color_t rgb2pix_1bpp(int r, int g, int b)
{
    const int gray = (2*r + 6*g + b) / 9;
    return gray < 128 ? 0 : 1;
}

/**
 * @brief Convert R, G, and B to 8 bit grayscale
 * @param r red value (0 … 255)
 * @param g green value (0 … 255)
 * @param b blue value (0 … 255)
 * @return grayscale value (0 … 255)
 */
static color_t rgb2pix_8bpp(int r, int g, int b)
{
    return (color_t)((2*r + 6*g + b) / 9);
}

/**
 * @brief Convert R, G, and B to 8 bit grayscale
 * @param r red value (0 … 255)
 * @param g green value (0 … 255)
 * @param b blue value (0 … 255)
 * @return RGB565 value (0 … 65535)
 */
static color_t rgb2pix_16bpp(int r, int g, int b)
{
    return (color_t)
	    (((b >> 3) & 0x1f) <<  0) |
	    (((g >> 2) & 0x3f) <<  5) |
	    (((r >> 3) & 0x1f) << 11);
}

/**
 * @brief Convert R, G, and B to 24 bit RGB 8-8-8
 * @param r red value (0 … 255)
 * @param g green value (0 … 255)
 * @param b blue value (0 … 255)
 * @return RGB888 value
 */
static color_t rgb2pix_24bpp(int r, int g, int b)
{
    return (color_t)
	    ((b & 0xff) <<  0) |
	    ((g & 0xff) <<  8) |
	    ((r & 0xff) << 16);
}

/**
 * @brief Convert R, G, and B to 24 bit ARGB32
 * @param r red value (0 … 255)
 * @param g green value (0 … 255)
 * @param b blue value (0 … 255)
 * @return RGB888 value
 */
static color_t rgb2pix_32bpp(int r, int g, int b)
{
    return (color_t)
	    ((b & 0xff) <<  0) |
	    ((g & 0xff) <<  8) |
	    ((r & 0xff) << 16) |
	    (0xff000000);
}

/**
 * @brief check if coordinates x and y are in range
 * 0 <= x < w and 0 <= y < h
 * otherwise return 0
 */
#define	CHECK_RANGE_GETPIXEL(_fb) do {	\
    if (x < 0 ||			\
	x >= (_fb)->w ||		\
	y < 0 ||			\
	y >= (_fb)->h) {		\
	return 0;			\
    }					\
} while (0)

/**
 * @brief Read a pixel value from the coordinates @p x and @p y
 * The frame buffer has 1 bit per pixel
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @return pixel value
 */
static color_t getpixel_1bpp(sfb_t* fb, int x, int y)
{
    CHECK_RANGE_GETPIXEL(fb);

    off_t pos =
	    ((x + fb->x) + 7) / 8 +
	    (y + fb->y) * fb->stride;

    return (fb->fbp[pos] >> (x & 7)) & 1;
}

/**
 * @brief Read a pixel value from the coordinates @p x and @p y
 * The frame buffer has 8 bits per pixel (color index or gray scale)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @return pixel value
 */
static color_t getpixel_8bpp(sfb_t* fb, int x, int y)
{
    CHECK_RANGE_GETPIXEL(fb);

    off_t pos =
	    (x + fb->x) +
	    (y + fb->y) * fb->stride;

    return fb->fbp[pos];
}

/**
 * @brief Read a pixel value from the coordinates @p x and @p y
 * The frame buffer has 16 bits per pixel (RGB 5-6-5)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @return pixel value
 */
static color_t getpixel_16bpp(sfb_t* fb, int x, int y)
{
    CHECK_RANGE_GETPIXEL(fb);

    off_t pos =
	    (x + fb->x) * 2 +
	    (y + fb->y) * fb->stride;

    return (((uint16_t)fb->fbp[pos+0] << 0) |
	    ((uint16_t)fb->fbp[pos+1] << 8));
}

/**
 * @brief Read a pixel value from the coordinates @p x and @p y
 * The frame buffer has 24 bits per pixel (RGB 8-8-8)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @return pixel value
 */
static color_t getpixel_24bpp(sfb_t* fb, int x, int y)
{
    CHECK_RANGE_GETPIXEL(fb);

    off_t pos =
	    (x + fb->x) * 3 +
	    (y + fb->y) * fb->stride;

    return (((uint32_t)fb->fbp[pos+0] <<  0) |
	    ((uint32_t)fb->fbp[pos+1] <<  8) |
	    ((uint32_t)fb->fbp[pos+2] << 16));
}

/**
 * @brief Read a pixel value from the coordinates @p x and @p y
 * The frame buffer has 32 bits per pixel (ARGB 8-8-8-8)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @return pixel value
 */
static color_t getpixel_32bpp(sfb_t* fb, int x, int y)
{
    CHECK_RANGE_GETPIXEL(fb);

    off_t pos =
	    (x + fb->x) * 4 +
	    (y + fb->y) * fb->stride;

    return (((uint32_t)fb->fbp[pos+0] <<  0) |
	    ((uint32_t)fb->fbp[pos+1] <<  8) |
	    ((uint32_t)fb->fbp[pos+2] << 16) |
	    ((uint32_t)fb->fbp[pos+3] << 24));
}

/**
 * @brief check if coordinates x and y are in range
 * 0 <= x < w and 0 <= y < h
 * otherwise return
 */
#define	CHECK_RANGE_SETPIXEL(_fb) do {	\
    if (x < 0 ||			\
	x >= (_fb)->w ||		\
	y < 0 ||			\
	y >= (_fb)->h) {		\
	return;				\
    }					\
} while (0)

/**
 * @brief Write a pixel value at the coordinates @p x and @p y
 * The frame buffer has 1 bit per pixel
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 */
static void setpixel_1bpp(sfb_t* fb, int x, int y)
{
    CHECK_RANGE_SETPIXEL(fb);

    off_t pos =
	    ((x + fb->x) + 7) / 8 +
	    (y + fb->y) * fb->stride;

    if (fb->fgcolor) {
	fb->fbp[pos] |= (0x80 >> (x & 7));
    } else {
	fb->fbp[pos] &= ~(0x80 >> (x & 7));
    }
}

/**
 * @brief Write a pixel value at the coordinates @p x and @p y
 * The frame buffer has 8 bits per pixel
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 */
static void setpixel_8bpp(sfb_t* fb, int x, int y)
{
    CHECK_RANGE_SETPIXEL(fb);

    off_t pos =
	    (x + fb->x) +
	    (y + fb->y) * fb->stride;;

    fb->fbp[pos] = (uint8_t) fb->fgcolor;
}

/**
 * @brief Write a pixel value at the coordinates @p x and @p y
 * The frame buffer has 16 bits per pixel (RGB 5-6-5)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 */
static void setpixel_16bpp(sfb_t* fb, int x, int y)
{
    CHECK_RANGE_SETPIXEL(fb);

    off_t pos =
	    (x + fb->x) * 2 +
	    (y + fb->y) * fb->stride;;

    fb->fbp[pos+0] = (uint8_t)(fb->fgcolor >> 0);
    fb->fbp[pos+1] = (uint8_t)(fb->fgcolor >> 8);
}

/**
 * @brief Write a pixel value at the coordinates @p x and @p y
 * The frame buffer has 24 bits per pixel (RGB 8-8-8)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 */
static void setpixel_24bpp(sfb_t* fb, int x, int y)
{
    CHECK_RANGE_SETPIXEL(fb);

    off_t pos =
	    (x + fb->x) * 3 +
	    (y + fb->y) * fb->stride;;

    fb->fbp[pos+0] = (uint8_t)(fb->fgcolor >>  0);
    fb->fbp[pos+1] = (uint8_t)(fb->fgcolor >>  8);
    fb->fbp[pos+2] = (uint8_t)(fb->fgcolor >> 16);
}

/**
 * @brief Write a pixel value at the coordinates @p x and @p y
 * The frame buffer has 32 bits per pixel (ARGB 8-8-8-8)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 */
static void setpixel_32bpp(sfb_t* fb, int x, int y)
{
    CHECK_RANGE_SETPIXEL(fb);

    off_t pos =
	    (x + fb->x) * 4 +
	    (y + fb->y) * fb->stride;;

    fb->fbp[pos+0] = (uint8_t)(fb->fgcolor >>  0);
    fb->fbp[pos+1] = (uint8_t)(fb->fgcolor >>  8);
    fb->fbp[pos+2] = (uint8_t)(fb->fgcolor >> 16);
    fb->fbp[pos+3] = (uint8_t)(fb->fgcolor >> 24);
}

/**
 * @brief check if coordinates x and y are in range
 * adjust l if x < 0 or x + l > w
 * 0 <= x < w and 0 <= y < h and l > 0
 * otherwise return
 */
#define	CHECK_RANGE_HLINE(_fb) do { \
    if (x < 0) {		    \
	l += x;			    \
	x = 0;			    \
        if (l <= 0) {		    \
	    return;		    \
        }                           \
    }				    \
    if (x + l >= fb->w) {	    \
	l = fb->w - x;		    \
    }				    \
    if (l <= 0 ||		    \
	x < 0 ||		    \
	x >= (_fb)->w ||	    \
	y < 0 ||		    \
	y >= (_fb)->h) {	    \
	return;			    \
    }				    \
} while (0)

/**
 * @brief Write a horizontal line at the coordinates @p x and @p y with length @p l
 * The frame buffer has 1 bit per pixel
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param l length in pixels
 */
static void hline_1bpp(sfb_t* fb, int x, int y, int l)
{
    CHECK_RANGE_HLINE(fb);

    off_t pos =
	    ((x + fb->x) + 7) / 8 +
	    (y + fb->y) * fb->stride;

    if (fb->fgcolor) {
	while (l-- > 0) {
	    fb->fbp[pos] |= (0x80 >> (x & 7));
	    x++;
	    if (0 == (x & 7))
		pos++;
	}
    } else {
	while (l-- > 0) {
	    fb->fbp[pos] &= ~(0x80 >> (x & 7));
	    x++;
	    if (0 == (x & 7))
		pos++;
	}
    }
}

/**
 * @brief Write a horizontal line at the coordinates @p x and @p y with length @p l
 * The frame buffer has 8 bits per pixel
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param l length in pixels
 */
static void hline_8bpp(sfb_t* fb, int x, int y, int l)
{
    CHECK_RANGE_HLINE(fb);

    off_t pos =
	    (x + fb->x) +
	    (y + fb->y) * fb->stride;;

    while (l-- > 0) {
	fb->fbp[pos++] = (uint8_t) fb->fgcolor;
    }
}

/**
 * @brief Write a horizontal line at the coordinates @p x and @p y with length @p l
 * The frame buffer has 16 bits per pixel (RGB 5-6-5)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param l length in pixels
 */
static void hline_16bpp(sfb_t* fb, int x, int y, int l)
{
    CHECK_RANGE_HLINE(fb);

    off_t pos =
	    (x + fb->x) * 2 +
	    (y + fb->y) * fb->stride;;

    while (l-- > 0) {
	fb->fbp[pos+0] = (uint8_t)(fb->fgcolor >> 0);
	fb->fbp[pos+1] = (uint8_t)(fb->fgcolor >> 8);
	pos += 2;
    }
}

/**
 * @brief Write a horizontal line at the coordinates @p x and @p y with length @p l
 * The frame buffer has 24 bits per pixel (RGB 8-8-8)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param l length in pixels
 */
static void hline_24bpp(sfb_t* fb, int x, int y, int l)
{
    CHECK_RANGE_HLINE(fb);

    off_t pos =
	    (x + fb->x) * 3 +
	    (y + fb->y) * fb->stride;;

    while (l-- > 0) {
	fb->fbp[pos+0] = (uint8_t)(fb->fgcolor >>  0);
	fb->fbp[pos+1] = (uint8_t)(fb->fgcolor >>  8);
	fb->fbp[pos+2] = (uint8_t)(fb->fgcolor >> 16);
	pos += 3;
    }
}

/**
 * @brief Write a horizontal line at the coordinates @p x and @p y with length @p l
 * The frame buffer has 32 bits per pixel (ARGB 8-8-8-8)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param l length in pixels
 */
static void hline_32bpp(sfb_t* fb, int x, int y, int l)
{
    CHECK_RANGE_HLINE(fb);

    off_t pos =
	    (x + fb->x) * 4 +
	    (y + fb->y) * fb->stride;;

    while (l-- > 0) {
	fb->fbp[pos+0] = (uint8_t)(fb->fgcolor >>  0);
	fb->fbp[pos+1] = (uint8_t)(fb->fgcolor >>  8);
	fb->fbp[pos+2] = (uint8_t)(fb->fgcolor >> 16);
	fb->fbp[pos+3] = (uint8_t)(fb->fgcolor >> 24);
	pos += 4;
    }
}

/**
 * @brief check if coordinates x and y are in range
 * adjust l if y < 0 or y + l > h
 * 0 <= x < w and 0 <= y < h and l > 0
 * otherwise return
 */
#define	CHECK_RANGE_VLINE(_fb) do { \
    if (y < 0) {		    \
	l += y;			    \
	y = 0;			    \
        if (l <= 0) {		    \
	    return;		    \
        }                           \
    }				    \
    if (y + l >= (_fb)->h) {	    \
	l = (_fb)->h - y;	    \
    }				    \
    if (l <= 0 ||		    \
	x < 0 ||		    \
	x >= (_fb)->w ||	    \
	y < 0 ||		    \
	y >= (_fb)->h) {	    \
	return;			    \
    }				    \
} while (0)

/**
 * @brief Write a vertical line at the coordinates @p x and @p y with length @p l
 * The frame buffer has 1 bit per pixel
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param l length in pixels
 */
static void vline_1bpp(sfb_t* fb, int x, int y, int l)
{
    CHECK_RANGE_VLINE(fb);

    off_t pos =
	    ((x + fb->x) + 7) / 8 +
	    (y + fb->y) * fb->stride;

    if (fb->fgcolor) {
	while (l-- > 0) {
	    fb->fbp[pos] |= (0x80 >> (x & 7));
	    pos += fb->stride;
	}
    } else {
	while (l-- > 0) {
	    fb->fbp[pos] &= ~(0x80 >> (x & 7));
	    pos += fb->stride;
	}
    }
}

/**
 * @brief Write a vertical line at the coordinates @p x and @p y with length @p l
 * The frame buffer has 8 bits per pixel
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param l length in pixels
 */
static void vline_8bpp(sfb_t* fb, int x, int y, int l)
{
    CHECK_RANGE_VLINE(fb);

    off_t pos =
	    (x + fb->x) +
	    (y + fb->y) * fb->stride;;

    while (l-- > 0) {
	fb->fbp[pos] = (uint8_t) fb->fgcolor;
	pos += fb->stride;
    }
}

/**
 * @brief Write a vertical line at the coordinates @p x and @p y with length @p l
 * The frame buffer has 16 bits per pixel (RGB 5-6-5)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param l length in pixels
 */
static void vline_16bpp(sfb_t* fb, int x, int y, int l)
{
    CHECK_RANGE_VLINE(fb);

    off_t pos =
	    (x + fb->x) * 2 +
	    (y + fb->y) * fb->stride;;

    while (l-- > 0) {
	fb->fbp[pos+0] = (uint8_t)(fb->fgcolor >> 0);
	fb->fbp[pos+1] = (uint8_t)(fb->fgcolor >> 8);
	pos += fb->stride;
    }
}

/**
 * @brief Write a vertical line at the coordinates @p x and @p y with length @p l
 * The frame buffer has 24 bits per pixel (RGB 8-8-8)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param l length in pixels
 */
static void vline_24bpp(sfb_t* fb, int x, int y, int l)
{
    CHECK_RANGE_VLINE(fb);

    off_t pos =
	    (x + fb->x) * 3 +
	    (y + fb->y) * fb->stride;;

    while (l-- > 0) {
	fb->fbp[pos+0] = (uint8_t)(fb->fgcolor >>  0);
	fb->fbp[pos+1] = (uint8_t)(fb->fgcolor >>  8);
	fb->fbp[pos+2] = (uint8_t)(fb->fgcolor >> 16);
	pos += fb->stride;
    }
}

/**
 * @brief Write a vertical line at the coordinates @p x and @p y with length @p l
 * The frame buffer has 32 bits per pixel (ARGB 8-8-8-8)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param l length in pixels
 */
static void vline_32bpp(sfb_t* fb, int x, int y, int l)
{
    CHECK_RANGE_VLINE(fb);

    off_t pos =
	    (x + fb->x) * 4 +
	    (y + fb->y) * fb->stride;;

    while (l-- > 0) {
	fb->fbp[pos+0] = (uint8_t)(fb->fgcolor >>  0);
	fb->fbp[pos+1] = (uint8_t)(fb->fgcolor >>  8);
	fb->fbp[pos+2] = (uint8_t)(fb->fgcolor >> 16);
	fb->fbp[pos+3] = (uint8_t)(fb->fgcolor >> 24);
	pos += fb->stride;
    }
}

/**
 * @brief Draw a line from @p x1, @p y1 to @p x2, @p y2
 *
 * @param fb pointer to the frame buffer context
 * @param x1 line start x coordinate
 * @param y1 line start y coordinate
 * @param x2 line end x coordinate
 * @param y2 line end y coordinate
 */
void fb_line(sfb_t *fb, int x1, int y1, int x2, int y2)
{
    CHECK_FB(fb);
    const int sx = x1 < x2 ? 1 : -1;
    const int sy = y1 < y2 ? 1 : -1;
    const int dx = abs(x2 - x1);
    const int dy = abs(y2 - y1);

    if (dx >= dy) {
	// Loop for x coordinates
	int dda = dx / 2;
	while (x1 != x2) {
	    fb->setpixel(fb, x1, y1);
	    x1 += sx;
	    dda -= dy;
	    if (dda <= 0) {
		y1 += sy;
		dda += dx;
	    }
	}
    } else {
	// Loop for y coordinates
	int dda = dy / 2;
	while (y1 != y2) {
	    fb->setpixel(fb, x1, y1);
	    y1 += sy;
	    dda -= dx;
	    if (dda <= 0) {
		x1 += sx;
		dda += dy;
	    }
	}
    }
}

/**
 * @brief Draw a rectangle at @p x1, @p y1 to @p x2, @p y2
 *
 * @param fb pointer to the frame buffer context
 * @param x1 first corner x coordinate
 * @param y1 first corner y coordinate
 * @param x2 opposite corner x coordinate
 * @param y2 opposite corner y coordinate
 */
void fb_rect(sfb_t *fb, int x1, int y1, int x2, int y2)
{
    CHECK_FB(fb);
    const int tl_x = x1 <= x2 ? x1 : x2;
    const int tl_y = y1 <= y2 ? y1 : y2;
    const int br_x = x1 > x2 ? x1 : x2;
    const int br_y = y1 > y2 ? y1 : y2;
    const int w = br_x + 1 - tl_x;
    const int h = br_y + 1 - tl_y;

    fb->hline(fb, tl_x, tl_y, w);
    fb->hline(fb, tl_x, br_y, w);
    fb->vline(fb, tl_x, tl_y, h);
    fb->vline(fb, br_x, tl_y, h);
}

/**
 * @brief Fill a rectangle at @p x1, @p y1 to @p x2, @p y2
 *
 * @param fb pointer to the frame buffer context
 * @param x1 first corner x coordinate
 * @param y1 first corner y coordinate
 * @param x2 opposite corner x coordinate
 * @param y2 opposite corner y coordinate
 */
void fb_fill(sfb_t *fb, int x1, int y1, int x2, int y2)
{
    CHECK_FB(fb);
    const int tl_x = x1 <= x2 ? x1 : x2;
    const int tl_y = y1 <= y2 ? y1 : y2;
    const int br_x = x1 > x2 ? x1 : x2;
    const int br_y = y1 > y2 ? y1 : y2;
    const int w = br_x + 1 - tl_x;
    const int h = br_y + 1 - tl_y;

    for (int i = 0; i < h; i++)
	fb->hline(fb, tl_x, tl_y + i, w);
}

/**
 * @brief Draw a circle's octants @p oct at @p x, @p y with radius @p r
 *
 * @param fb pointer to the frame buffer context
 * @param oct octants to draw (0 … 7 for counter-clockwise octants)
 * @param x center x coordinate
 * @param y center y coordinate
 * @param r radius in pixels
 */
void fb_circle_octants(sfb_t *fb, uint8_t oct, int x, int y, int r)
{
    CHECK_FB(fb);
    int dda = r;
    int dx = r;
    int dy = 0;
    while (dx >= dy) {
	if (oct & (1 << 0))
	    fb->setpixel(fb, x + dx, y - dy);
	if (oct & (1 << 1))
	    fb->setpixel(fb, x + dy, y - dx);
	if (oct & (1 << 2))
	    fb->setpixel(fb, x - dy, y - dx);
	if (oct & (1 << 3))
	    fb->setpixel(fb, x - dx, y - dy);
	if (oct & (1 << 4))
	    fb->setpixel(fb, x - dx, y + dy);
	if (oct & (1 << 5))
	    fb->setpixel(fb, x - dy, y + dx);
	if (oct & (1 << 6))
	    fb->setpixel(fb, x + dy, y + dx);
	if (oct & (1 << 7))
	    fb->setpixel(fb, x + dx, y + dy);

	dy++;
	dda -= dy;
	if (dda < 0) {
	    dda += dx;
	    dx--;
	}
    }
}

/**
 * @brief Draw a circle at @p x, @p y with radius @p r
 *
 * @param fb pointer to the frame buffer context
 * @param x center x coordinate
 * @param y center y coordinate
 * @param r radius in pixels
 */
void fb_circle(sfb_t *fb, int x, int y, int r)
{
    CHECK_FB(fb);
    fb_circle_octants(fb, 0xff, x, y, r);
}

/**
 * @brief Draw a disc's octants @p oct at @p x, @p y with radius @p r
 *
 * @param fb pointer to the frame buffer context
 * @param oct octants to draw (0 … 7 for counter-clockwise octants)
 * @param x center x coordinate
 * @param y center y coordinate
 * @param r radius in pixels
 */
void fb_disc_octants(sfb_t *fb, uint8_t oct, int x, int y, int r)
{
    CHECK_FB(fb);
    int dda = r;
    int dx = r;
    int dy = 0;
    while (dx >= dy) {
	const int l = dx - dy;
	if (oct & (1 << 0))
	    fb->hline(fb, x + dy, y - dy, l);
	if (oct & (1 << 1))
	    fb->vline(fb, x + dy, y - dx, l);
	if (oct & (1 << 2))
	    fb->vline(fb, x - dy, y - dx, l);
	if (oct & (1 << 3))
	    fb->hline(fb, x - dx, y - dy, l + 1); // FIXME: + 1 or there is a gap?
	if (oct & (1 << 4))
	    fb->hline(fb, x - dx, y + dy, l);
	if (oct & (1 << 5))
	    fb->vline(fb, x - dy, y + dy, l);
	if (oct & (1 << 6))
	    fb->vline(fb, x + dy, y + dy, l);
	if (oct & (1 << 7))
	    fb->hline(fb, x + dy, y + dy, l);
	dy++;
	dda -= dy;
	if (dda < 0) {
	    dda += dx;
	    dx--;
	}
    }
}

/**
 * @brief Draw a dist at @p x, @p y with radius @p r
 *
 * @param fb pointer to the frame buffer context
 * @param x center x coordinate
 * @param y center y coordinate
 * @param r radius in pixels
 */
void fb_disc(sfb_t *fb, int x, int y, int r)
{
    CHECK_FB(fb);
    fb_disc_octants(fb, 0xff, x, y, r);
}

/**
 * @brief Initialize the framebuffer device info and map to memory
 * @param sfb pointer to the frame buffer context pointer
 * @param devname device name like "/dev/fb1"
 * @return 0 on success, or < 0 on error
 */
int fb_init(struct sfb_s** sfb, const char* devname)
{
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    sfb_t* fb = (sfb_t *)calloc(1, sizeof(sfb_t));

    *sfb = NULL;
    fb->magic = SFB_MAGIC;
    fb->fbp = MAP_FAILED;
    fb->font = &font_10x20;

    fb->fd = open(devname, O_RDWR);
    if (-1 == fb->fd) {
	free(fb);
	return -1;
    }

    // Get fixed screen information
    if (ioctl(fb->fd, FBIOGET_FSCREENINFO, &finfo) == -1) {
	free(fb);
	return -2;
    }

    // Get variable screen information
    if (ioctl(fb->fd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
	free(fb);
	return -3;
    }
    fb->devname = devname;

    // Figure out the width, height, bpp, size and stridt of the frame buffer
    fb->w = vinfo.xres;
    fb->h = vinfo.yres;
    fb->bpp = vinfo.bits_per_pixel;
    fb->size = fb->w * fb->h * fb->bpp / 8;
    fb->stride = finfo.line_length;

    // Try to memory map the framebuffer
    // Map the device to memory
    fb->fbp = (uint8_t *) mmap(0, fb->size, PROT_READ | PROT_WRITE, MAP_SHARED, fb->fd, 0);
    if (MAP_FAILED == fb->fbp) {
	close(fb->fd);
	free(fb);
	return -4;
    }

    // Figure out which pixel getter/setter to use
    switch (fb->bpp) {
    case 1:
	fb->rgb2pix = rgb2pix_1bpp;
	fb->getpixel = getpixel_1bpp;
	fb->setpixel = setpixel_1bpp;
	fb->hline = hline_1bpp;
	fb->vline = vline_1bpp;
	break;
    case 8:
	fb->rgb2pix = rgb2pix_8bpp;
	fb->getpixel = getpixel_8bpp;
	fb->setpixel = setpixel_8bpp;
	fb->hline = hline_8bpp;
	fb->vline = vline_8bpp;
	break;
    case 16:
	fb->rgb2pix = rgb2pix_16bpp;
	fb->getpixel = getpixel_16bpp;
	fb->setpixel = setpixel_16bpp;
	fb->hline = hline_16bpp;
	fb->vline = vline_16bpp;
	break;
    case 24:
	fb->rgb2pix = rgb2pix_24bpp;
	fb->getpixel = getpixel_24bpp;
	fb->setpixel = setpixel_24bpp;
	fb->hline = hline_24bpp;
	fb->vline = vline_24bpp;
	break;
    case 32:
	fb->rgb2pix = rgb2pix_32bpp;
	fb->getpixel = getpixel_32bpp;
	fb->setpixel = setpixel_32bpp;
	fb->hline = hline_32bpp;
	fb->vline = vline_32bpp;
	break;
    default:
	munmap(fb->fbp, fb->size);
	close(fb->fd);
	free(fb);
	return -5;
    }
    fb->bgcolor = fb_color2pixel(fb, color_Black);
    fb->fgcolor = fb_color2pixel(fb, color_White);
    fb->opaque = 1;

    *sfb = fb;

    return 0;
}

/**
 * @brief Unmap memory and close the framebuffer device
 * @param sfb pointer to the frame buffer context pointer
 */
void fb_exit(sfb_t** sfb)
{
    if (!sfb)
	return;

    sfb_t* fb = *sfb;
    *sfb = NULL;
    CHECK_FB(fb);

    if (MAP_FAILED != fb->fbp) {
	munmap(fb->fbp, fb->size);
	fb->fbp = MAP_FAILED;
    }
    if (fb->fd >= 0) {
	close(fb->fd);
	fb->fd = -1;
    }
    free(fb);
}

/**
 * @brief Select one of the integrated fonts
 * @param fb pointer to the frame buffer context
 * @param efont font_e enumeration value
 */
void fb_set_font(sfb_t* fb, font_e efont)
{
    switch (efont) {
    case Font_6x12:
        fb->font = &font_6x12;
        break;
    case Font_8x13:
        fb->font = &font_8x13;
        break;
    case Font_9x15:
        fb->font = &font_9x15;
        break;
    case Font_10x20:
        fb->font = &font_10x20;
        break;
    }
}

/**
 * @brief Return the framebuffer device name
 * @param fb pointer to the frame buffer context
 */
const char* fb_devname(sfb_t* fb)
{
    CHECK_FB_RET(fb, NULL);
    return fb->devname;
}

/**
 * @brief Return framebuffer x offset
 * @param fb pointer to the frame buffer context
 */
int fb_x(sfb_t* fb)
{
    CHECK_FB_RET(fb, 0);
    return fb->x;
}

/**
 * @brief Return framebuffer y offset
 * @param fb pointer to the frame buffer context
 */
int fb_y(sfb_t* fb)
{
    CHECK_FB_RET(fb, 0);
    return fb->y;
}

/**
 * @brief Return width of framebuffer
 * @param fb pointer to the frame buffer context
 */
int fb_w(sfb_t* fb)
{
    CHECK_FB_RET(fb, 0);
    return fb->w;
}

/**
 * @brief Return height of framebuffer
 * @param fb pointer to the frame buffer context
 */
int fb_h(sfb_t* fb)
{
    CHECK_FB_RET(fb, 0);
    return fb->h;
}

/**
 * @brief Return number of bits per pixel for the framebuffer
 * @param fb pointer to the frame buffer context
 */
int fb_bpp(sfb_t* fb)
{
    CHECK_FB_RET(fb, 0);
    return fb->bpp;
}

/**
 * @brief Return cursor x coordinate
 * @param fb pointer to the frame buffer context
 */
int fb_cx(sfb_t* fb)
{
    CHECK_FB_RET(fb, 0);
    return fb->cursor_x;
}

/**
 * @brief Return cursor y coordinate
 * @param fb pointer to the frame buffer context
 */
int fb_cy(sfb_t* fb)
{
    CHECK_FB_RET(fb, 0);
    return fb->cursor_y;
}

/**
 * @brief Return width of the font
 * @param fb pointer to the frame buffer context
 */
int fb_font_w(sfb_t* fb)
{
    CHECK_FB_RET(fb, 0);
    return fb->font->w;
}

/**
 * @brief Return height of the font
 * @param fb pointer to the frame buffer context
 */
int fb_font_h(sfb_t* fb)
{
    CHECK_FB_RET(fb, 0);
    return fb->font->h;
}

/**
 * @brief Go to cursor @p x and @p y coordinates
 * @param fb pointer to the frame buffer context
 * @param x new cursor x coordinate
 * @param y new cursor y coordinate
 */
void fb_gotoxy(sfb_t* fb, int x, int y)
{
    CHECK_FB(fb);
    fb->cursor_x = BOUND(x, 0, fb->w - 1);
    fb->cursor_y = BOUND(y, 0, fb->h - 1);
}

/**
 * @brief Return the background mode
 * @param fb pointer to the frame buffer context
 * @return background mode: 0 transparent, 1 opaque
 */
int fb_opaque(sfb_t* fb)
{
    CHECK_FB_RET(fb, 0);
    return fb->opaque;
}

/**
 * @brief Return the background color
 * @param fb pointer to the frame buffer context
 * @return background color
 */
color_t fb_bgcolor(sfb_t* fb)
{
    CHECK_FB_RET(fb, 0);
    return fb->bgcolor;
}

/**
 * @brief Return the foreground color
 * @param fb pointer to the frame buffer context
 * @return foreground color
 */
color_t fb_fgcolor(sfb_t* fb)
{
    CHECK_FB_RET(fb, 0);
    return fb->fgcolor;
}

/**
 * @brief Set the background mode
 * @param fb pointer to the frame buffer context
 * @param mode background mode to set (0: transparent, 1: opaque)
 */
void fb_set_opaque(sfb_t* fb, int mode)
{
    CHECK_FB(fb);
    fb->opaque = mode & 1;
}

/**
 * @brief Set the background color
 * @param fb pointer to the frame buffer context
 * @param bg background color to set
 */
void fb_set_bgcolor(sfb_t* fb, color_t bg)
{
    CHECK_FB(fb);
    fb->bgcolor = bg;
}

/**
 * @brief Set the foreground color
 * @param fb pointer to the frame buffer context
 * @param fg foreground color to set
 */
void fb_set_fgcolor(sfb_t* fb, color_t fg)
{
    CHECK_FB(fb);
    fb->fgcolor = fg;
}

/**
 * @brief Convert RGB triple to a pixel value (color_t)
 * @param fb pointer to the frame buffer context
 * @param r red value (0 … 255)
 * @param g green value (0 … 255)
 * @param b blue value (0 … 255)
 * @return color_t pixel value for the frame buffer depth
 */
color_t fb_rgb2pixel(sfb_t* fb, int r, int g, int b)
{
    CHECK_FB_RET(fb, (color_t)~0u);
    return fb->rgb2pix(r, g, b);
}

/**
 * @brief Convert color_e value @p color to a pixel value (color_t)
 * @param fb pointer to the frame buffer context
 * @return color_t pixel value for the frame buffer depth
 */
color_t fb_color2pixel(sfb_t* fb, color_e color)
{
    CHECK_FB_RET(fb, (color_t)~0u);
    const int r = (color >>  0) & 0xff;
    const int g = (color >>  8) & 0xff;
    const int b = (color >> 16) & 0xff;
    return fb->rgb2pix(r, g, b);
}

/**
 * @brief Clear the framebuffer
 * @param fb pointer to the frame buffer context
 */
void fb_clear(sfb_t* fb)
{
    CHECK_FB(fb);
    assert(fb->fbp != MAP_FAILED);
    switch (fb->bpp) {
    case 1:
	memset(fb->fbp, fb->bgcolor ? 0xff : 0x00, fb->size);
	break;
    case 8:
	memset(fb->fbp, fb->bgcolor, fb->size);
	break;
    case 16:
	for (off_t off = 0; off < fb->size; off += 2) {
	    fb->fbp[off+0] = (uint8_t)(fb->bgcolor >> 0);
	    fb->fbp[off+1] = (uint8_t)(fb->bgcolor >> 8);
	}
	break;
    case 24:
	for (off_t off = 0; off < fb->size; off += 3) {
	    fb->fbp[off+0] = (uint8_t)(fb->bgcolor >>  0);
	    fb->fbp[off+1] = (uint8_t)(fb->bgcolor >>  8);
	    fb->fbp[off+2] = (uint8_t)(fb->bgcolor >> 16);
	}
	break;
    case 32:
	for (off_t off = 0; off < fb->size; off += 4) {
	    fb->fbp[off+0] = (uint8_t)(fb->bgcolor >>  0);
	    fb->fbp[off+1] = (uint8_t)(fb->bgcolor >>  8);
	    fb->fbp[off+2] = (uint8_t)(fb->bgcolor >> 16);
	    fb->fbp[off+3] = 0xff;
	}
	break;
    }
}

/**
 * @brief Shift the frame buffer in one direction
 * @param fb pointer to the frame buffer context
 * @param dir shift direction
 * @param pixels number of pixels to shift
 */
void fb_shift(sfb_t* fb, shift_dir_e dir, int pixels)
{
    CHECK_FB(fb);
    switch (dir) {
    case shift_left:	/* to the left */
	for (int y = 0; y < fb->h; y++) {
	    const int pos = y * fb->stride;
	    memmove(&fb->fbp[pos], &fb->fbp[pos+pixels], fb->stride - pixels);
	    memset(&fb->fbp[pos+pixels], 0, pixels);
	}
	break;
    case shift_right:	/* to the right */
	for (int y = 0; y < fb->h; y++) {
	    const int pos = y * fb->stride;
	    memmove(&fb->fbp[pos+pixels], &fb->fbp[pos], fb->stride - pixels);
	    memset(&fb->fbp[pos], 0, pixels);
	}
	break;
    case shift_up:	/* to the top */
	memmove(fb->fbp, &fb->fbp[fb->stride * pixels],
		fb->stride * (fb->h - pixels));
	memset(&fb->fbp[fb->stride * (fb->h - pixels)], 0,
		fb->stride * pixels);
	break;
    case shift_down: /* to the bottom */
    default:
	memmove(&fb->fbp[fb->stride * pixels], fb->fbp,
		fb->stride * (fb->h - pixels));
	memset(fb->fbp, 0, fb->stride * pixels);
	break;
    }
}

/**
 * @brief Read a pixel value from the coordinates @p x and @p y
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @return pixel value
 */
color_t fb_getpixel(sfb_t* fb, int x, int y)
{
    CHECK_FB_RET(fb, (color_t)~0u);
    return fb->getpixel(fb, x, y);
}

/**
 * @brief Write a pixel value @p color to the coordinates @p x and @p y
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 */
void fb_setpixel(sfb_t* fb, int x, int y)
{
    CHECK_FB(fb);
    return fb->setpixel(fb, x, y);
}

/**
 * @brief Write a horizontal line at the coordinates @p x and @p y with length @p l
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param l length in pixels
 */
void fb_hline(sfb_t* fb, int x, int y, int l)
{
    CHECK_FB(fb);
    return fb->hline(fb, x, y, l);
}

/**
 * @brief Write a vertical line at the coordinates @p x and @p y with length @p l
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param l length in pixels
 */
void fb_vline(sfb_t* fb, int x, int y, int l)
{
    CHECK_FB(fb);
    return fb->vline(fb, x, y, l);
}

/**
 * @brief Put a character glyph into the framebuffer
 * @param fb pointer to the frame buffer context
 * @param wc wide character code (unicode) to put
 */
void fb_putc(sfb_t* fb, wchar_t wc)
{
    CHECK_FB(fb);
    const fbfont_t* font = fb->font;
    uint32_t glyph = 0;
    for (uint32_t i = 0; font->map[i]; i++) {
	if (wc != font->map[i])
	    continue;
	glyph = i;
	break;
    }

    const off_t offs = font->h * glyph;
    if (fb->opaque) {
	swap_fg_bg(fb);
	/* Opaque mode: fill the glyph rectangle */
	fb_fill(fb,
	    fb->cursor_x, fb->cursor_y,
	    fb->cursor_x + font->w - 1, fb->cursor_y + font->h - 1);
	swap_fg_bg(fb);
    }

    if (font->w <= 8) {
	/* One uint8_t per glyph row */
        const uint8_t* data = (const uint8_t *)font->data;
        for (int y0 = 0; y0 < font->h; y0++) {
	    uint8_t bits = data[offs+y0] << (8 - font->w);
	    for (int x0 = 0; bits && x0 < font->w; bits <<= 1, x0++) {
	        if (bits & 0x80)
		    fb->setpixel(fb, fb->cursor_x + x0, fb->cursor_y + y0);
	    }
        }
    } else if (font->w <= 16) {
	/* One uint16_t per glyph row */
	const uint16_t* data = (const uint16_t *)font->data;
        for (int y0 = 0; y0 < font->h; y0++) {
	    uint16_t bits = data[offs+y0] << (16 - font->w);
	    for (int x0 = 0; bits && x0 < font->w; bits <<= 1, x0++) {
	        if (bits & 0x8000)
		    fb->setpixel(fb, fb->cursor_x + x0, fb->cursor_y + y0);
	    }
        }
    } else {
	/* One uint32_t per glyph row */
	const uint32_t* data = (const uint32_t *)font->data;
	for (int y0 = 0; y0 < font->h; y0++) {
	    uint32_t bits = data[offs+y0] << (32 - font->w);
	    for (int x0 = 0; bits && x0 < font->w; bits <<= 1, x0++) {
		if (bits & 0x80000000ul)
		    fb->setpixel(fb, fb->cursor_x + x0, fb->cursor_y + y0);
	    }
	}
    }
}

/**
 * @brief Put a string @p text into the framebuffer using @p color
 *
 * Some control characters are handled:
 *  '\\n' carriage return and line feed (cursor_x = 0, cursor_y += fh, scroll if off screen)
 *  '\\r' carriage return (cursor_x = 0)
 *
 * @param fb pointer to the frame buffer context
 * @param text pointer to NUL terminated string to put
 */
size_t fb_puts(sfb_t* fb, const char* text)
{
    CHECK_FB_RET(fb, (size_t)-1);
    const fbfont_t* font = fb->font;

    const size_t n = mbstowcs(NULL, text, 0) + 1;
    if ((size_t)-1 == n) {
	error(fb, "Invalid UTF-8 encoding in '%s'", text);
	return n;
    }

    wchar_t* wtext = (wchar_t *) calloc(n, sizeof(wchar_t));
    const size_t size = mbstowcs(wtext, text, n);

    if ((size_t)-1 == size) {
	error(fb, "Invalid UTF-8 encoding in '%s'", text);
	return size;
    }

    if (0 == size) {
	error(fb, "Zero length result for '%s'", text);
	return size;
    }

    for (size_t i = 0; i < size; i++) {
	int advance = 0;

	switch (wtext[i]) {
	case 0x000a:  /* new line (also does carriage return) */
	    fb->cursor_x = 0;
	    fb->cursor_y += font->h;
	    /* need to scroll the frame buffer? */
	    if (fb->cursor_y + font->h > fb->h) {
		fb_shift(fb, shift_up, font->h);
		fb->cursor_y -= font->h;
	    }
	    break;

	case 0x000c:  /* form feed */
	    fb_clear(fb);
	    fb->cursor_x = fb->cursor_y = 0;
	    break;

	case 0x000d:  /* carriage return */
	    fb->cursor_x = 0;
	    break;

	default:
	    fb_putc(fb, wtext[i]);
	    advance = 1;
	    break;
	}

	if (!advance) {
	    continue;
	}

	fb->cursor_x += font->w;
	if (fb->cursor_x + font->w >= fb->w) {
	    fb->cursor_x = 0;
	    fb->cursor_y += font->h;
	    if (fb->cursor_y + font->h >= fb->h) {
		fb->cursor_y -= font->h;
		fb_shift(fb, shift_up, font->h);
	    }
	}
    }
    free(wtext);
    return size;
}

/**
 * @brief Print a format string and its arguments
 *
 * @param fb pointer to the frame buffer context
 * @param format C format string
 * @param ap a va_list of arguments
 */
static size_t fb_vprintf(sfb_t* fb, const char* format, va_list ap)
{
    CHECK_FB_RET(fb, (size_t)-1);

    size_t size = vsnprintf(NULL, 0, format, ap);
    char* buffer = (char *)calloc(1, size + 1);
    if (NULL == buffer) {
	error(fb, "Error: insufficient memory for fb_vprintf() (%u)", size);
	return (size_t)-1;
    }

    size = vsnprintf(buffer, size, format, ap);
    if ((int)size > 0) {
	fb_puts(fb, buffer);
    }
    free(buffer);
    return size;
}

/**
 * @brief Print a format string and optional arguments
 *
 * @param fb pointer to the frame buffer context
 * @param format C format string followed by optional arguments
 */
size_t fb_printf(sfb_t* fb, const char* format, ...)
{
    CHECK_FB_RET(fb, (size_t)-1);
    va_list ap;
    va_start(ap, format);
    size_t size = fb_vprintf(fb, format, ap);
    va_end(ap);
    return size;
}

/**
 * @brief Write a gd image to the framebuffer /dev/fb1
 *
 * @param fb pointer to the frame buffer context
 * @param im gdImagePtr with the image to write
 */
void fb_dump(sfb_t* fb, gdImagePtr im)
{
    CHECK_FB(fb);
    switch (fb->bpp) {
    case 1:
	for (int y = 0; y < fb->h; y++) {
	    const off_t pos = y * fb->stride;
	    uint8_t* dst = &fb->fbp[pos];
            uint8_t bits = 0;
	    for (int x = 0; x < fb->w; x++) {
		int pix = gdImageGetPixel(im, x, y);
                if (pix)
                    bits |= (0x80 >> (x % 8));
                else
                    bits &= ~(0x80 >> (x % 8));
                if (7 == (x % 8)) {
                    *dst++ = bits;
                }
	    }
            if (0 != (fb->w % 8)) {
                *dst = bits;
            }
	}
        break;
    case 8:
	for (int y = 0; y < fb->h; y++) {
	    const off_t pos = y * fb->stride;
	    uint8_t* dst = &fb->fbp[pos];
	    for (int x = 0; x < fb->w; x++) {
		int pix = gdImageGetPixel(im, x, y);
		*dst++ = (uint8_t)pix;
	    }
	}
        break;
    case 16:
	for (int y = 0; y < fb->h; y++) {
	    const off_t pos = y * fb->stride;
	    uint8_t* dst = &fb->fbp[pos];
	    for (int x = 0; x < fb->w; x++) {
		/* convert truecolor to RGB 565 */
		int pix = gdImageGetTrueColorPixel(im, x, y);
		const uint16_t r = gdTrueColorGetRed(pix);
		const uint16_t g = gdTrueColorGetGreen(pix);
		const uint16_t b = gdTrueColorGetBlue(pix);
		const color_t p = fb->rgb2pix(r, g, b);
		dst[0] = (uint8_t)(p >> 0);
		dst[1] = (uint8_t)(p >> 8);
		dst += 2;
	    }
	}
        break;
    case 24:
	for (int y = 0; y < fb->h; y++) {
	    const off_t pos = y * fb->stride;
	    uint8_t* dst = &fb->fbp[pos];
	    for (int x = 0; x < fb->w; x++) {
		int pix = gdImageGetTrueColorPixel(im, x, y);
		dst[0] = (uint8_t)(pix >>  0);
		dst[1] = (uint8_t)(pix >>  8);
		dst[2] = (uint8_t)(pix >> 16);
		dst += 3;
	    }
	}
        break;
    case 32:
	for (int y = 0; y < fb->h; y++) {
	    const off_t pos = y * fb->stride;
	    uint8_t* dst = &fb->fbp[pos];
	    for (int x = 0; x < fb->w; x++) {
		int pix = gdImageGetTrueColorPixel(im, x, y);
		dst[0] = (uint8_t)(pix >>  0);
		dst[1] = (uint8_t)(pix >>  8);
		dst[2] = (uint8_t)(pix >> 16);
		dst[3] = 0xff;
		dst += 4;
	    }
	}
        break;
    }
}
