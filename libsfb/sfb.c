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

typedef struct sfb_s {
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

    /** @brief pointer to the function to get a pixel for a specific depth */
    uint32_t (*getpixel)(struct sfb_s* sfb, int x, int y);

    /** @brief pointer to the function to set a pixel for a specific depth */
    void (*setpixel)(struct sfb_s* sfb, int x, int y, color_t color);

    /** @brief pointer to the function to write a horizontal line for a specific depth */
    void (*hline)(struct sfb_s* sfb, int x, int y, int l, color_t color);

    /** @brief pointer to the function to write a vertical line for a specific depth */
    void (*vline)(struct sfb_s* sfb, int x, int y, int l, color_t color);

    /** @brief pointer to font to use */
    const fbfont_t* font;

    /** @brief cursor x coordinate */
    int curx;

    /** @brief cursor y coordinate */
    int cury;
}   sfb_t;

/**
 * @brief Print an error message into the frame buffer context's buffer
 * @param fb pointer to the frame buffer context
 * @param format C format string followed by optional parameters
 */
static void error(sfb_t* fb, const char* format, ...)
{
    va_list ap;
    va_start(ap, format);
    vsnprintf(fb->errmsg, sizeof(fb->errmsg), format, ap);
    va_end(ap);
}

/**
 * @brief check if coordinates x and y are in range
 * 0 <= x < w and 0 <= y < h
 * otherwise return 0
 */
#define	RANGE_CHECK_GETPIXEL(_fb) do {	\
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
    RANGE_CHECK_GETPIXEL(fb);

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
    RANGE_CHECK_GETPIXEL(fb);

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
    RANGE_CHECK_GETPIXEL(fb);

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
    RANGE_CHECK_GETPIXEL(fb);

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
    RANGE_CHECK_GETPIXEL(fb);

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
#define	RANGE_CHECK_SETPIXEL(_fb) do {	\
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
 * @param color pixel color to set
 */
static void setpixel_1bpp(sfb_t* fb, int x, int y, color_t color)
{
    RANGE_CHECK_SETPIXEL(fb);

    off_t pos =
	    ((x + fb->x) + 7) / 8 +
	    (y + fb->y) * fb->stride;

    if (color) {
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
 * @param color pixel color to set
 */
static void setpixel_8bpp(sfb_t* fb, int x, int y, color_t color)
{
    RANGE_CHECK_SETPIXEL(fb);

    off_t pos =
	    (x + fb->x) +
	    (y + fb->y) * fb->stride;;

    fb->fbp[pos] = (uint8_t) color;
}

/**
 * @brief Write a pixel value at the coordinates @p x and @p y
 * The frame buffer has 16 bits per pixel (RGB 5-6-5)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param color pixel color to set
 */
static void setpixel_16bpp(sfb_t* fb, int x, int y, color_t color)
{
    RANGE_CHECK_SETPIXEL(fb);

    off_t pos =
	    (x + fb->x) * 2 +
	    (y + fb->y) * fb->stride;;

    fb->fbp[pos+0] = (uint8_t)(color >> 0);
    fb->fbp[pos+1] = (uint8_t)(color >> 8);
}

/**
 * @brief Write a pixel value at the coordinates @p x and @p y
 * The frame buffer has 24 bits per pixel (RGB 8-8-8)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param color pixel color to set
 */
static void setpixel_24bpp(sfb_t* fb, int x, int y, color_t color)
{
    RANGE_CHECK_SETPIXEL(fb);

    off_t pos =
	    (x + fb->x) * 3 +
	    (y + fb->y) * fb->stride;;

    fb->fbp[pos+0] = (uint8_t)(color >>  0);
    fb->fbp[pos+1] = (uint8_t)(color >>  8);
    fb->fbp[pos+2] = (uint8_t)(color >> 16);
}

/**
 * @brief Write a pixel value at the coordinates @p x and @p y
 * The frame buffer has 32 bits per pixel (ARGB 8-8-8-8)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param color pixel color to set
 */
static void setpixel_32bpp(sfb_t* fb, int x, int y, color_t color)
{
    RANGE_CHECK_SETPIXEL(fb);

    off_t pos =
	    (x + fb->x) * 4 +
	    (y + fb->y) * fb->stride;;

    fb->fbp[pos+0] = (uint8_t)(color >>  0);
    fb->fbp[pos+1] = (uint8_t)(color >>  8);
    fb->fbp[pos+2] = (uint8_t)(color >> 16);
    fb->fbp[pos+3] = (uint8_t)(color >> 24);
}

/**
 * @brief check if coordinates x and y are in range
 * adjust l if x < 0 or x + l > w
 * 0 <= x < w and 0 <= y < h and l > 0
 * otherwise return
 */
#define	RANGE_CHECK_HLINE(_fb) do { \
    if (x < 0) {		    \
	l += x;			    \
	x = 0;			    \
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
 * @param color pixel color to set
 */
static void hline_1bpp(sfb_t* fb, int x, int y, int l, color_t color)
{
    RANGE_CHECK_HLINE(fb);

    off_t pos =
	    ((x + fb->x) + 7) / 8 +
	    (y + fb->y) * fb->stride;

    if (x + l >= fb->w) {
	l = fb->w - x;
    }

    if (color) {
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
 * @param color pixel color to set
 */
static void hline_8bpp(sfb_t* fb, int x, int y, int l, color_t color)
{
    RANGE_CHECK_HLINE(fb);

    off_t pos =
	    (x + fb->x) +
	    (y + fb->y) * fb->stride;;

    if (x + l >= fb->w) {
	l = fb->w - x;
    }

    while (l-- > 0) {
	fb->fbp[pos++] = (uint8_t) color;
    }
}

/**
 * @brief Write a horizontal line at the coordinates @p x and @p y with length @p l
 * The frame buffer has 16 bits per pixel (RGB 5-6-5)
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param l length in pixels
 * @param color pixel color to set
 */
static void hline_16bpp(sfb_t* fb, int x, int y, int l, color_t color)
{
    RANGE_CHECK_HLINE(fb);

    off_t pos =
	    (x + fb->x) * 2 +
	    (y + fb->y) * fb->stride;;

    if (x + l >= fb->w) {
	l = fb->w - x;
    }

    while (l-- > 0) {
	fb->fbp[pos+0] = (uint8_t)(color >> 0);
	fb->fbp[pos+1] = (uint8_t)(color >> 8);
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
 * @param color pixel color to set
 */
static void hline_24bpp(sfb_t* fb, int x, int y, int l, color_t color)
{
    RANGE_CHECK_HLINE(fb);

    off_t pos =
	    (x + fb->x) * 3 +
	    (y + fb->y) * fb->stride;;

    if (x + l >= fb->w) {
	l = fb->w - x;
    }

    while (l-- > 0) {
	fb->fbp[pos+0] = (uint8_t)(color >>  0);
	fb->fbp[pos+1] = (uint8_t)(color >>  8);
	fb->fbp[pos+2] = (uint8_t)(color >> 16);
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
 * @param color pixel color to set
 */
static void hline_32bpp(sfb_t* fb, int x, int y, int l, color_t color)
{
    RANGE_CHECK_HLINE(fb);

    off_t pos =
	    (x + fb->x) * 4 +
	    (y + fb->y) * fb->stride;;

    if (x + l >= fb->w) {
	l = fb->w - x;
    }

    while (l-- > 0) {
	fb->fbp[pos+0] = (uint8_t)(color >>  0);
	fb->fbp[pos+1] = (uint8_t)(color >>  8);
	fb->fbp[pos+2] = (uint8_t)(color >> 16);
	fb->fbp[pos+3] = (uint8_t)(color >> 24);
	pos += 4;
    }
}

/**
 * @brief check if coordinates x and y are in range
 * adjust l if y < 0 or y + l > h
 * 0 <= x < w and 0 <= y < h and l > 0
 * otherwise return
 */
#define	RANGE_CHECK_VLINE(_fb) do { \
    if (y < 0) {		    \
	l += y;			    \
	y = 0;			    \
    }				    \
    if (y + l >= (_fb)->h) {	    \
	l = (_fb)->w - x;	    \
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
 * @param color pixel color to set
 */
static void vline_1bpp(sfb_t* fb, int x, int y, int l, color_t color)
{
    RANGE_CHECK_VLINE(fb);

    off_t pos =
	    ((x + fb->x) + 7) / 8 +
	    (y + fb->y) * fb->stride;

    if (y + l >= fb->h) {
	l = fb->h - y;
    }

    if (color) {
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
 * @param color pixel color to set
 */
static void vline_8bpp(sfb_t* fb, int x, int y, int l, color_t color)
{
    RANGE_CHECK_VLINE(fb);

    off_t pos =
	    (x + fb->x) +
	    (y + fb->y) * fb->stride;;

    if (y + l >= fb->h) {
	l = fb->h - y;
    }

    while (l-- > 0) {
	fb->fbp[pos] = (uint8_t) color;
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
 * @param color pixel color to set
 */
static void vline_16bpp(sfb_t* fb, int x, int y, int l, color_t color)
{
    RANGE_CHECK_VLINE(fb);

    off_t pos =
	    (x + fb->x) * 2 +
	    (y + fb->y) * fb->stride;;

    if (y + l >= fb->h) {
	l = fb->h - y;
    }

    while (l-- > 0) {
	fb->fbp[pos+0] = (uint8_t)(color >> 0);
	fb->fbp[pos+1] = (uint8_t)(color >> 8);
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
 * @param color pixel color to set
 */
static void vline_24bpp(sfb_t* fb, int x, int y, int l, color_t color)
{
    RANGE_CHECK_VLINE(fb);

    off_t pos =
	    (x + fb->x) * 3 +
	    (y + fb->y) * fb->stride;;

    if (y + l >= fb->h) {
	l = fb->h - y;
    }

    while (l-- > 0) {
	fb->fbp[pos+0] = (uint8_t)(color >>  0);
	fb->fbp[pos+1] = (uint8_t)(color >>  8);
	fb->fbp[pos+2] = (uint8_t)(color >> 16);
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
 * @param color pixel color to set
 */
static void vline_32bpp(sfb_t* fb, int x, int y, int l, color_t color)
{
    RANGE_CHECK_VLINE(fb);

    off_t pos =
	    (x + fb->x) * 4 +
	    (y + fb->y) * fb->stride;;

    if (y + l >= fb->h) {
	l = fb->h - y;
    }

    while (l-- > 0) {
	fb->fbp[pos+0] = (uint8_t)(color >>  0);
	fb->fbp[pos+1] = (uint8_t)(color >>  8);
	fb->fbp[pos+2] = (uint8_t)(color >> 16);
	fb->fbp[pos+3] = (uint8_t)(color >> 24);
	pos += fb->stride;
    }
}

/**
 * @brief Draw a line from @p x1, @p y1 to @p x2, @p y2
 *
 * @param x1 line start x coordinate
 * @param y1 line start y coordinate
 * @param x2 line end x coordinate
 * @param y2 line end y coordinate
 * @param color pixel color
 */
void fb_line(sfb_t *fb, int x1, int y1, int x2, int y2, color_t color)
{
    const int sx = x1 < x2 ? 1 : -1;
    const int sy = y1 < y2 ? 1 : -1;
    const int dx = abs(x2 - x1);
    const int dy = abs(y2 - y1);

    if (dx >= dy) {
	// Loop for x coordinates
	int dda = dx / 2;
	while (x1 != x2) {
	    fb->setpixel(fb, x1, y1, color);
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
	    fb->setpixel(fb, x1, y1, color);
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
 * @param x1 first corner x coordinate
 * @param y1 first corner y coordinate
 * @param x2 opposite corner x coordinate
 * @param y2 opposite corner y coordinate
 * @param color pixel color
 */
void fb_rect(sfb_t *fb, int x1, int y1, int x2, int y2, color_t color)
{
    const int tl_x = x1 <= x2 ? x1 : x2;
    const int tl_y = y1 <= y2 ? y1 : y2;
    const int br_x = x1 > x2 ? x1 : x2;
    const int br_y = y1 > y2 ? y1 : y2;
    const int w = br_x + 1 - tl_x;
    const int h = br_y + 1 - tl_y;

    fb->hline(fb, tl_x, tl_y, w, color);
    fb->hline(fb, tl_x, br_y, w, color);
    fb->vline(fb, tl_x, tl_y, h, color);
    fb->vline(fb, br_x, tl_y, h, color);
}

/**
 * @brief Fill a rectangle at @p x1, @p y1 to @p x2, @p y2
 *
 * @param x1 first corner x coordinate
 * @param y1 first corner y coordinate
 * @param x2 opposite corner x coordinate
 * @param y2 opposite corner y coordinate
 * @param color pixel color
 */
void fb_fill(sfb_t *fb, int x1, int y1, int x2, int y2, color_t color)
{
    const int tl_x = x1 <= x2 ? x1 : x2;
    const int tl_y = y1 <= y2 ? y1 : y2;
    const int br_x = x1 > x2 ? x1 : x2;
    const int br_y = y1 > y2 ? y1 : y2;
    const int w = br_x + 1 - tl_x;
    const int h = br_y + 1 - tl_y;

    for (int i = 0; i < h; i++)
	fb->hline(fb, tl_x, tl_y + i, w, color);
}

/**
 * @brief Initialize the framebuffer device info and map to memory
 * @param fb pointer to the frame buffer context
 * @param devname device name like "/dev/fb1"
 * @return 0 on success, or < 0 on error
 */
int fb_init(struct sfb_s** sfb, const char* devname)
{
    struct fb_fix_screeninfo finfo;
    struct fb_var_screeninfo vinfo;
    sfb_t* fb = (sfb_t *)calloc(1, sizeof(sfb_t));

    *sfb = NULL;
    fb->fbp = MAP_FAILED;
    fb->font = &font_6x8;

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
	fb->getpixel = getpixel_1bpp;
	fb->setpixel = setpixel_1bpp;
	fb->hline = hline_1bpp;
	fb->vline = vline_1bpp;
	break;
    case 8:
	fb->getpixel = getpixel_8bpp;
	fb->setpixel = setpixel_8bpp;
	fb->hline = hline_8bpp;
	fb->vline = vline_8bpp;
	break;
    case 16:
	fb->getpixel = getpixel_16bpp;
	fb->setpixel = setpixel_16bpp;
	fb->hline = hline_16bpp;
	fb->vline = vline_16bpp;
	break;
    case 24:
	fb->getpixel = getpixel_24bpp;
	fb->setpixel = setpixel_24bpp;
	fb->hline = hline_24bpp;
	fb->vline = vline_24bpp;
	break;
    case 32:
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
    if (fb) {
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
    *sfb = NULL;
}

/**
 * @brief Return the framebuffer device name
 * @param fb pointer to the frame buffer context
 */
const char* fb_devname(sfb_t* fb)
{
    return fb->devname;
}

/**
 * @brief Return framebuffer x offset
 * @param fb pointer to the frame buffer context
 */
int fb_x(sfb_t* fb)
{
    if (!fb)
	return 0;
    return fb->x;
}

/**
 * @brief Return framebuffer y offset
 * @param fb pointer to the frame buffer context
 */
int fb_y(sfb_t* fb)
{
    if (!fb)
	return 0;
    return fb->y;
}

/**
 * @brief Return width of framebuffer
 * @param fb pointer to the frame buffer context
 */
int fb_w(sfb_t* fb)
{
    if (!fb)
	return 0;
    return fb->w;
}

/**
 * @brief Return height of framebuffer
 * @param fb pointer to the frame buffer context
 */
int fb_h(sfb_t* fb)
{
    if (!fb)
	return 0;
    return fb->h;
}

/**
 * @brief Return number of bits per pixel for the framebuffer
 * @param fb pointer to the frame buffer context
 */
int fb_bpp(sfb_t* fb)
{
    if (!fb)
	return 0;
    return fb->bpp;
}

/**
 * @brief Return cursor x coordinate
 * @param fb pointer to the frame buffer context
 */
int fb_cx(sfb_t* fb)
{
    if (!fb)
	return 0;
    return fb->curx;
}

/**
 * @brief Return cursor y coordinate
 * @param fb pointer to the frame buffer context
 */
int fb_cy(sfb_t* fb)
{
    if (!fb)
	return 0;
    return fb->cury;
}

/**
 * @brief Go to cursor @px and @p y coordinates
 * @param fb pointer to the frame buffer context
 */
void fb_gotoxy(sfb_t* fb, int x, int y)
{
    if (!fb)
	return;
    fb->curx = BOUND(x, 0, fb->w - 1);
    fb->cury = BOUND(y, 0, fb->h - 1);
}

/**
 * @brief Clear the framebuffer
 * @param fb pointer to the frame buffer context
 */
void fb_clear(sfb_t* fb)
{
    if (!fb)
	return;
    assert(fb->fbp != MAP_FAILED);
    memset(fb->fbp, 0, fb->size);
}

/**
 * @brief Shift the frame buffer in one direction
 * @param fb pointer to the frame buffer context
 */
void fb_shift(sfb_t* fb, shift_dir_e dir, int pixels)
{
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
    if (!fb)
	return 0;
    return fb->getpixel(fb, x, y);
}

/**
 * @brief Write a pixel value @p color to the coordinates @p x and @p y
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @paral color pixel color
 * @return pixel value
 */
void fb_setpixel(sfb_t* fb, int x, int y, color_t color)
{
    if (!fb)
	return;
    return fb->setpixel(fb, x, y, color);
}

/**
 * @brief Write a horizontal line at the coordinates @p x and @p y with length @p l
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param l length in pixels
 * @param color pixel color to set
 */
void fb_hline(sfb_t* fb, int x, int y, int l, color_t color)
{
    if (!fb)
	return;
    return fb->hline(fb, x, y, l, color);
}

/**
 * @brief Write a vertical line at the coordinates @p x and @p y with length @p l
 * @param fb pointer to the frame buffer context
 * @param x coordinate
 * @param y coordinate
 * @param l length in pixels
 * @param color pixel color to set
 */
void fb_vline(sfb_t* fb, int x, int y, int l, color_t color)
{
    if (!fb)
	return;
    return fb->vline(fb, x, y, l, color);
}

/**
 * @brief Put a character glyph into the framebuffer
 * @param fb pointer to the frame buffer context
 * @param color pixel color to set
 * @param ch ISO-8859-1 character code to put
 */
void fb_putc(sfb_t* fb, color_t color, char c)
{
    uint8_t ch = (uint8_t)c;

    if (ch >= fb->font->count)
	return;

    const size_t offs = fb->font->h * (uint8_t)ch;

    for (int y0 = 0; y0 < fb->font->h; y0++) {
	uint8_t bits = fb->font->data[offs+y0] << (8 - fb->font->w);
	for (int x0 = 0; bits && x0 < fb->font->w; bits <<= 1, x0++) {
	    if (bits & 0x80)
		fb->setpixel(fb, fb->curx + x0, fb->cury + y0, color);
	}
    }
}

/**
 * @brief Put a string @p text into the framebuffer using @p color
 *
 * Some control characters are handled:
 *  '\n' carriage return and line feed (curx = 0, cury += fh, scroll if off screen)
 *  '\r' carriage return (curx = 0)
 *
 * @param fb pointer to the frame buffer context
 * @param color pixel color to set
 * @param text pointer to NUL terminated string to put
 */
void fb_puts(sfb_t* fb, color_t color, const char* text)
{
    for (int i = 0; text[i]; i++) {
	int advance = 0;

	switch (text[i]) {
	case '\n':
	    fb->curx = 0;
	    fb->cury += fb->font->h;
	    if (fb->cury + fb->font->h >= fb->h) {
		fb->cury -= fb->font->h;
		fb_shift(fb, shift_up, fb->font->h);
	    }
	    break;

	case 13:
	    fb->curx = 0;
	    break;

	default:
	    fb_putc(fb, color, (uint8_t)text[i]);
	    advance = 1;
	    break;
	}

	if (!advance) {
	    continue;
	}

	fb->curx += fb->font->w;
	if (fb->curx + fb->font->w >= fb->w) {
	    fb->curx = 0;
	    fb->cury += fb->font->h;
	    if (fb->cury + fb->font->h >= fb->h) {
		fb->cury -= fb->font->h;
		fb_shift(fb, shift_up, fb->font->h);
	    }
	}
    }
}

/**
 * @brief Print a format string and its arguments
 *
 * @param fb pointer to the frame buffer context
 * @param format C format string
 * @param ap a va_list of arguments
 */
static size_t fb_vprintf(sfb_t* fb, color_t color, const char* format, va_list ap)
{
    if (!fb)
        return (size_t)-1;

    size_t size = vsnprintf(NULL, 0, format, ap);
    char* buffer = (char *)calloc(1, size + 1);
    if (!buffer) {
	error(fb, "Error: insufficient memory for fb_vprintf() (%u)", size);
	return (size_t)-1;
    }
    size = vsnprintf(buffer, size, format, ap);
    if ((int)size > 0) {
        fb_puts(fb, color, buffer);
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
size_t fb_printf(sfb_t* fb, color_t color, const char* format, ...)
{
    size_t size;
    va_list ap;
    va_start(ap, format);
    size = fb_vprintf(fb, color, format, ap);
    va_end(ap);
    return size;
}

/**
 * @brief convert a RGB 8-8-8 triplet to a RGB 5-6-5 value
 * @param r red value
 * @param g green value
 * @param b blue value
 * @param 16 bit value RRRRRGGGGGGBBBBB
 */
static inline uint16_t rgb32_to_16(const unsigned r, const unsigned g, const unsigned b)
{
    return (uint16_t)
	    (((b >> 3) & 0x1f) <<  0) |
	    (((g >> 2) & 0x3f) <<  5) |
	    (((r >> 3) & 0x1f) << 11);
}

/**
 * @brief Write a gd image to the framebuffer /dev/fb1
 *
 * @param im gdImagePtr with the image to write
 */
void fb_dump(sfb_t* fb, gdImagePtr im)
{
    if (16 == fb->bpp) {
	for (int y = 0; y < fb->h; y++) {
	    const off_t pos = y * fb->stride;
	    uint8_t* dst = &fb->fbp[pos];
	    for (int x = 0; x < fb->w; x++) {
		/* convert truecolor to RGB 565 */
		int pix = gdImageGetTrueColorPixel(im, x, y);
		const uint16_t r = gdTrueColorGetRed(pix);
		const uint16_t g = gdTrueColorGetGreen(pix);
		const uint16_t b = gdTrueColorGetBlue(pix);
		const uint16_t p = rgb32_to_16(r, g, b);
		dst[0] = (uint8_t)(p >> 0);
		dst[1] = (uint8_t)(p >> 8);
		dst += 2;
	    }
	}
    } else {
	for (int y = 0; y < fb->h; y++) {
	    const off_t pos = y * fb->stride;
	    uint8_t* dst = &fb->fbp[pos];
	    for (int x = 0; x < fb->w; x++) {
		int pix = gdImageGetTrueColorPixel(im, x, y);
		dst[0] = (uint8_t)(pix >>  0);
		dst[1] = (uint8_t)(pix >>  8);
		dst[2] = (uint8_t)(pix >> 16);
		dst += 4;
	    }
	}
    }
}
