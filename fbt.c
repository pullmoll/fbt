/******************************************************************************
 * Copyright (c) Jürgen Buchmüller <pullmoll@t-online.de>
 * All rights reserved.
 *
 * fbt.c - Example program to write to a TFT framebuff
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

#define	DEFAULT_FB  "/dev/fb1"

static int verbose = 0;
static const fbfont_t* font = &font_6x8;
static const char* fb_devname = NULL;
static struct fb_fix_screeninfo finfo;
static struct fb_var_screeninfo vinfo;
static int fbfd = -1;
static int fb_w = 0;
static int fb_h = 0;
static int fb_bpp = 0;
static size_t fb_size = 0;
static size_t fb_stride = 0;
static uint8_t *fbp = MAP_FAILED;
static int curx = 0;
static int cury = 0;

static void info(int priority, const char* format, ...)
{
    va_list ap;

    if (priority <= verbose)
	return;
    va_start(ap, format);
    vfprintf(stdout, format, ap);
    va_end(ap);
}


/**
 * @brief Callback for libgd in case of an error
 */
static void gd_error(int priority, const char* format, va_list ap)
{
    info(priority, format, ap);
}

/**
 * @brief Poor man's basename
 * @param filename to strip the path from
 * @return basename, i.e. filename and suffix(es)
 */
static const char* basename(const char* filename)
{
    const char* slash = strrchr(filename, '/');
    return slash ? slash + 1 : filename;
}

/**
 * @brief Read a pixel value from the coordinates @p x and @p y
 * The frame buffer has 1 bit per pixel
 * @param x coordinate
 * @param y coordinate
 * @return pixel value
 */
static uint32_t fb_getpixel_1bpp(int x, int y)
{
    off_t pos =
	    ((x + vinfo.xoffset) + 7) / 8 +
	    (y + vinfo.yoffset) * fb_stride;

    if (x < 0 || x >= fb_w || y < 0 || y >= fb_h) {
	return 0;
    }
    return (fbp[pos] >> (x & 7)) & 1;
}

/**
 * @brief Read a pixel value from the coordinates @p x and @p y
 * The frame buffer has 8 bits per pixel (color index or gray scale)
 * @param x coordinate
 * @param y coordinate
 * @return pixel value
 */
static uint32_t fb_getpixel_8bpp(int x, int y)
{
    off_t pos =
	    (x + vinfo.xoffset) +
	    (y + vinfo.yoffset) * fb_stride;

    if (x < 0 || x >= fb_w || y < 0 || y >= fb_h) {
	return 0;
    }
    return fbp[pos];
}

/**
 * @brief Read a pixel value from the coordinates @p x and @p y
 * The frame buffer has 16 bits per pixel (RGB 5-6-5)
 * @param x coordinate
 * @param y coordinate
 * @return pixel value
 */
static uint32_t fb_getpixel_16bpp(int x, int y)
{
    off_t pos =
	    (x + vinfo.xoffset) * 2 +
	    (y + vinfo.yoffset) * fb_stride;

    if (x < 0 || x >= fb_w || y < 0 || y >= fb_h) {
	return 0;
    }
    return (((uint16_t)fbp[pos+0] << 0) |
	    ((uint16_t)fbp[pos+1] << 8));
}

/**
 * @brief Read a pixel value from the coordinates @p x and @p y
 * The frame buffer has 24 bits per pixel (RGB 8-8-8)
 * @param x coordinate
 * @param y coordinate
 * @return pixel value
 */
static uint32_t fb_getpixel_24bpp(int x, int y)
{
    off_t pos =
	    (x + vinfo.xoffset) * 3 +
	    (y + vinfo.yoffset) * fb_stride;

    if (x < 0 || x >= fb_w || y < 0 || y >= fb_h) {
	return 0;
    }
    return (((uint32_t)fbp[pos+0] <<  0) |
	    ((uint32_t)fbp[pos+1] <<  8) |
	    ((uint32_t)fbp[pos+2] << 16));
}

/**
 * @brief Read a pixel value from the coordinates @p x and @p y
 * The frame buffer has 32 bits per pixel (ARGB 8-8-8-8)
 * @param x coordinate
 * @param y coordinate
 * @return pixel value
 */
static uint32_t fb_getpixel_32bpp(int x, int y)
{
    off_t pos =
	    (x + vinfo.xoffset) * 4 +
	    (y + vinfo.yoffset) * fb_stride;

    if (x < 0 || x >= fb_w || y < 0 || y >= fb_h) {
	return 0;
    }
    return (((uint32_t)fbp[pos+0] <<  0) |
	    ((uint32_t)fbp[pos+1] <<  8) |
	    ((uint32_t)fbp[pos+2] << 16) |
	    ((uint32_t)fbp[pos+3] << 24));
}

/**
 * @brief Write a pixel value at the coordinates @p x and @p y
 * The frame buffer has 1 bit per pixel
 * @param x coordinate
 * @param y coordinate
 * @param color pixel color to set
 */
void fb_setpixel_1bpp(int x, int y, uint32_t color)
{
    off_t pos =
	    ((x + vinfo.xoffset) + 7) / 8 +
	    (y + vinfo.yoffset) * fb_stride;

    if (x < 0 || x >= fb_w || y < 0 || y >= fb_h) {
	return;
    }
    if (color) {
	fbp[pos] |= (0x80 >> (x & 7));
    } else {
	fbp[pos] &= ~(0x80 >> (x & 7));
    }
}

/**
 * @brief Write a pixel value at the coordinates @p x and @p y
 * The frame buffer has 8 bits per pixel
 * @param x coordinate
 * @param y coordinate
 * @param color pixel color to set
 */
void fb_setpixel_8bpp(int x, int y, uint32_t color)
{
    off_t pos =
	    (x + vinfo.xoffset) +
	    (y + vinfo.yoffset) * fb_stride;;

    if (x < 0 || x >= fb_w || y < 0 || y >= fb_h) {
	return;
    }
    fbp[pos] = (uint8_t) color;
}

/**
 * @brief Write a pixel value at the coordinates @p x and @p y
 * The frame buffer has 16 bits per pixel (RGB 5-6-5)
 * @param x coordinate
 * @param y coordinate
 * @param color pixel color to set
 */
void fb_setpixel_16bpp(int x, int y, uint32_t color)
{
    off_t pos =
	    (x + vinfo.xoffset) * 2 +
	    (y + vinfo.yoffset) * fb_stride;;

    if (x < 0 || x >= fb_w || y < 0 || y >= fb_h) {
	return;
    }
    fbp[pos+0] = (uint8_t)(color >> 0);
    fbp[pos+1] = (uint8_t)(color >> 8);
}

/**
 * @brief Write a pixel value at the coordinates @p x and @p y
 * The frame buffer has 24 bits per pixel (RGB 8-8-8)
 * @param x coordinate
 * @param y coordinate
 * @param color pixel color to set
 */
void fb_setpixel_24bpp(int x, int y, uint32_t color)
{
    off_t pos =
	    (x + vinfo.xoffset) * 3 +
	    (y + vinfo.yoffset) * fb_stride;;

    if (x < 0 || x >= fb_w || y < 0 || y >= fb_h) {
	return;
    }
    fbp[pos+0] = (uint8_t)(color >>  0);
    fbp[pos+1] = (uint8_t)(color >>  8);
    fbp[pos+2] = (uint8_t)(color >> 16);
}

/**
 * @brief Write a pixel value at the coordinates @p x and @p y
 * The frame buffer has 32 bits per pixel (ARGB 8-8-8-8)
 * @param x coordinate
 * @param y coordinate
 * @param color pixel color to set
 */
void fb_setpixel_32bpp(int x, int y, uint32_t color)
{
    off_t pos =
	    (x + vinfo.xoffset) * 4 +
	    (y + vinfo.yoffset) * fb_stride;;

    if (x < 0 || x >= fb_w || y < 0 || y >= fb_h) {
	return;
    }
    fbp[pos+0] = (uint8_t)(color >>  0);
    fbp[pos+1] = (uint8_t)(color >>  8);
    fbp[pos+2] = (uint8_t)(color >> 16);
    fbp[pos+3] = (uint8_t)(color >> 24);
}

/** @brief pointer to the function to get a pixel for a specific depth */
uint32_t (*fb_getpixel)(int x, int y) = fb_getpixel_32bpp;

/** @brief pointer to the function to set a pixel for a specific depth */
void (*fb_setpixel)(int x, int y, uint32_t color) = fb_setpixel_32bpp;

/**
 * @brief Draw a line from @p x1, @p y1 to @p x2, @p y2
 *
 * @param x1 line start x coordinate
 * @param y1 line start y coordinate
 * @param x2 line end x coordinate
 * @param y2 line end y coordinate
 * @param color pixel color
 */
static void fb_line(int x1, int y1, int x2, int y2, uint32_t color)
{
    const int sx = x1 < x2 ? 1 : -1;
    const int sy = y1 < y2 ? 1 : -1;
    const int dx = abs(x2 - x1);
    const int dy = abs(y2 - y1);

    if (dx >= dy) {
	// Loop for x coordinates
	int dda = dx / 2;
	int x = x1;
	int y = y1;
	while (x != x2) {
	    fb_setpixel(x, y, color);
	    x += sx;
	    dda -= dy;
	    if (dda <= 0) {
		y += sy;
		dda += dx;
	    }
	}
    } else {
	// Loop for y coordinates
	int dda = dy / 2;
	int x = x1;
	int y = y1;
	while (y != y2) {
	    fb_setpixel(x, y, color);
	    y += sy;
	    dda -= dx;
	    if (dda <= 0) {
		x += sx;
		dda += dy;
	    }
	}
    }
}

/**
 * @brief Initialize the framebuffer device info and map to memory
 * @param devname device name like "/dev/fb1"
 * @return 0 on success, or < 0 on error
 */
static int fb_init(const char* devname)
{
    fbfd = open(devname, O_RDWR);

    if (-1 == fbfd) {
        perror("Error: opening the framebuffer device with open(devname, O_RDWR)");
        return -1;
    }

    // Get fixed screen information
    if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo) == -1) {
	perror("Error: reading frame bufer fixed information");
        return -2;
    }

    // Get variable screen information
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo) == -1) {
	perror("Error: reading frame bufer variable information");
        return -3;
    }
    fb_devname = devname;

    // Figure out the width, height, bpp, size and stridt of the frame buffer
    fb_w = vinfo.xres;
    fb_h = vinfo.yres;
    fb_bpp = vinfo.bits_per_pixel;
    fb_size = fb_w * fb_h * fb_bpp / 8;
    fb_stride = finfo.line_length;

    // Figure out which pixel getter/setter to use
    switch (fb_bpp) {
    case 1:
	fb_getpixel = fb_getpixel_1bpp;
	fb_setpixel = fb_setpixel_1bpp;
	break;
    case 8:
	fb_getpixel = fb_getpixel_8bpp;
	fb_setpixel = fb_setpixel_8bpp;
	break;
    case 16:
	fb_getpixel = fb_getpixel_16bpp;
	fb_setpixel = fb_setpixel_16bpp;
	break;
    case 24:
	fb_getpixel = fb_getpixel_24bpp;
	fb_setpixel = fb_setpixel_24bpp;
	break;
    case 32:
	fb_getpixel = fb_getpixel_32bpp;
	fb_setpixel = fb_setpixel_32bpp;
	break;
    default:
	info(1, "Error: unsupported frame buffer bits-per-pixel %d\n", fb_bpp);
    }

    // Try to memory map the framebuffer
    // Map the device to memory
    fbp = (uint8_t *) mmap(0, fb_size, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, 0);
    if (MAP_FAILED == fbp) {
        perror("Error: failed to map framebuffer device to memory");
        return -4;
    }
    info(1, "The framebuffer device was mapped to memory at %p (%u).\n", fbp, fb_size);

    return 0;
}

/**
 * @brief Unmap memory and close the framebuffer device
 */
static void fb_exit(void)
{
    if (MAP_FAILED != fbp) {
        munmap(fbp, fb_size);
    }
    if (fbfd >= 0) {
        close(fbfd);
    }
}

/**
 * @brief Clear the framebuffer
 */
static void fb_clear(void)
{
    memset(fbp, 0, vinfo.yres * fb_stride);
}

/**
 * @brief Shift the frame buffer in one direction
 */
static void fb_shift(int dir, int pixels)
{
	switch (dir) {
	case 0:	/* to the left */
		for (int y = 0; y < vinfo.yres; y++) {
			const int pos = y * fb_stride;
			memmove(&fbp[pos], &fbp[pos+pixels], fb_stride - pixels);
			memset(&fbp[pos+pixels], 0, pixels);
		}
		break;
	case 1:	/* to the right */
		for (int y = 0; y < vinfo.yres; y++) {
			const int pos = y * fb_stride;
			memmove(&fbp[pos+pixels], &fbp[pos], fb_stride - pixels);
			memset(&fbp[pos], 0, pixels);
		}
		break;
	case 2:	/* to the top */
		memmove(fbp, &fbp[fb_stride * pixels],
			fb_stride * (vinfo.yres - pixels));
		memset(&fbp[fb_stride * (vinfo.yres - pixels)], 0,
			fb_stride * pixels);
		break;
	case 3: /* to the bottom */
	default:
		memmove(&fbp[fb_stride * pixels], fbp,
			fb_stride * (vinfo.yres - pixels));
		memset(fbp, 0, fb_stride * pixels);
		break;
	}
}

/**
 * @brief Put a character glyph into the framebuffer
 * @param x coordinate of top, left pixel
 * @param y coordinate of top, left pixel
 * @param color pixel color to set
 * @param ch ASCII character code to put
 */
static void fb_putchar(int x, int y, int color, uint8_t ch)
{
    if (ch >= font->count)
        return;

    const size_t offs = font->h * (uint8_t)ch;

    for (int y0 = 0; y0 < font->h; y0++) {
        uint8_t bits = font->data[offs+y0] << (8 - font->w);
        for (int x0 = 0; bits && x0 < font->w; bits <<= 1, x0++) {
            if (bits & 0x80)
                fb_setpixel(x + x0, y + y0, color);
        }
    }
}

/**
 * @brief Put a string @p text into the framebuffer at @p x, @p y with @p color
 * @param x pointer to coordinate of top, left pixel
 * @param y pointer to coordinate of top, left pixel
 * @param color pixel color to set
 * @param text pointer to NUL terminated string to put
 */
static void fb_puttext(int *x, int *y, int color, const char* text)
{
    for (int i = 0; text[i]; i++) {
        int advance = 0;
        switch (text[i]) {
        case 10:
            *x = 0;
            *y += font->h;
	    if (*y + font->h >= fb_h) {
	        *y -= font->h;
	        fb_shift(2, font->h);
            }
            break;

        case 13:
            *x = 0;
            break;

        default:
            fb_putchar(*x, *y, color, (uint8_t)text[i]);
            advance = 1;
            break;
        }

        if (advance) {
            *x += font->w;
            if (*x + font->w >= fb_w) {
                *x = 0;
                *y += font->h;
                if (*y + font->h >= fb_h) {
		    *y -= font->h;
		    fb_shift(2, font->h);
                }
            }
        }
    }
}

static size_t fb_vprintf(const char* format, va_list ap)
{
    size_t size = vsnprintf(NULL, 0, format, ap);
    char* buffer = malloc(size);
    if (NULL == buffer) {
        perror("fb_vprintf()");
        return (size_t)-1;
    }
    vsnprintf(buffer, size, format, ap);
    fb_puttext(&curx, &cury, 0x00ffffff, buffer);
    free(buffer);
    return size;
}

static size_t fb_printf(const char* format, ...)
{
    size_t size;
    va_list ap;
    va_start(ap, format);
    size = fb_vprintf(format, ap);
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
static inline uint16_t rgb32_to_16(const uint16_t r, const uint16_t g, const uint16_t b)
{
    return
        (((b >> 3) & 0x1f) <<  0) |
    	(((g >> 2) & 0x3f) <<  5) |
    	(((r >> 3) & 0x1f) << 11);
}

/**
 * @brief Write a gd image to the framebuffer /dev/fb1
 *
 * @param im gdImagePtr with the image to write
 */
void fb_dump(gdImagePtr im)
{
    if (16 == vinfo.bits_per_pixel) {
        for (int y = 0; y < fb_h; y++) {
            const int pos = y * fb_stride;
            uint8_t* dst = &fbp[pos];
            for (int x = 0; x < fb_w; x++) {
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
        for (int y = 0; y < fb_h; y++) {
            const int pos = y * fb_stride;
            uint8_t* dst = &fbp[pos];
            for (int x = 0; x < fb_w; x++) {
        	int pix = gdImageGetTrueColorPixel(im, x, y);
                dst[0] = (uint8_t)(pix >>  0);
                dst[1] = (uint8_t)(pix >>  8);
                dst[2] = (uint8_t)(pix >> 16);
                dst += 4;
            }
        }
    }
}
    
/**
 * @brief A simple test drawing lines across the TFT
 */
void test_lines(int us)
{
    for (int x = 0; x < fb_w; x += 3) {
	fb_line(x, 0, fb_w - 1 - x, fb_h - 1, 0xffffff);
	if (us) {
		usleep(us);
	}
    }

    for (int y = 0; y < fb_h; y += 3) {
	fb_line(0, y, fb_w - 1, fb_h - 1 - y, 0xfffffff);
	if (us) {
		usleep(us);
	}
    }
}

void test_text()
{
    static const char text[] =
    "To be, or not to be, that is the question:\n"
    "Whether 'tis nobler in the mind to suffer\n"
    "The slings and arrows of outrageous fortune,\n"
    "Or to take Arms against a Sea of troubles,\n"
    "And by opposing end them: to die, to sleep;\n"
    "No more; and by a sleep, to say we end\n"
    "The heart-ache, and the thousand natural shocks\n"
    "That Flesh is heir to? 'Tis a consummation\n"
    "Devoutly to be wished. To die, to sleep,\n"
    "perchance to Dream; aye, there's the rub,\n"
    "For in that sleep of death, what dreams may come,\n"
    "When we have shuffled off this mortal coil,\n"
    "Must give us pause. There's the respect\n"
    "That makes Calamity of so long life:\n"
    "For who would bear the Whips and Scorns of time,\n"
    "The Oppressor's wrong, the proud man's Contumely,\n"
    "The pangs of dispised Love, the Law’s delay,\n"
    "The insolence of Office, and the spurns\n"
    "That patient merit of the unworthy takes,\n"
    "When he himself might his Quietus make\n"
    "With a bare Bodkin? Who would Fardels bear, [F: these Fardels]\n"
    "To grunt and sweat under a weary life,\n"
    "But that the dread of something after death,\n"
    "The undiscovered country, from whose bourn\n"
    "No traveller returns, puzzles the will,\n"
    "And makes us rather bear those ills we have,\n"
    "Than fly to others that we know not of.\n"
    "Thus conscience does make cowards of us all,\n"
    "And thus the native hue of Resolution\n"
    "Is sicklied o'er, with the pale cast of Thought,\n"
    "And enterprises of great pitch and moment, [F: pith]\n"
    "With this regard their Currents turn awry, [F: away]\n"
    "And lose the name of Action. Soft you now,\n"
    "The fair Ophelia? Nymph, in thy Orisons\n"
    "Be all my sins remember'd.\n";

    for (const char* line = text; *line; line = strchr(line, '\n') + 1) {
        char buff[256];
        char* eol = strchr(line, '\n');
        if (eol) {
            size_t len = (size_t)(eol + 1 - line);
            memcpy(buff, line, len);
            buff[len] = '\0';
            fb_printf(buff);
        } else {
            fb_printf(line);
        }
        usleep(100 * 1000);
    }
}

/**
 * @brief Load an image file and display it
 * @param filename of the image
 * @param upscale if non zero, scale smaller images up to the TFT dimensions
 */
void load_image(const char* filename, int upscale)
{
    char buff[128], *suffix;
    gdImagePtr im1, im2;

    im1 = gdImageCreateFromFile(filename);
    if (NULL == im1) {
        printf("Loading %s failed\n", filename);
    	return;
    }

    const int w_src = gdImageSX(im1);
    const int h_src = gdImageSY(im1);
    info(1, "Loaded image '%s' %dx%d %dbpp\n", filename, w_src, h_src, gdImageTrueColor(im1) ? 32 : 8);

    im2 = gdImageCreateTrueColor(fb_w, fb_h);
    int w_dst = w_src;
    int h_dst = h_src;
    if (upscale || w_src > fb_w || h_src > fb_h) {
        if (fb_w > fb_h) {
		w_dst = fb_w;
		h_dst = h_src * fb_w / w_src;
		if (h_dst > fb_h) {
			h_dst = fb_h;
			w_dst = w_src * fb_h / h_src;
		}
        } else {
		h_dst = fb_h;
		w_dst = w_src * fb_w / h_src;
		if (w_dst > fb_w) {
			w_dst = fb_w;
			h_dst = h_src * fb_w / w_src;
		}
        }
	const int x_dst = (fb_w - w_dst) / 2;
	const int y_dst = (fb_h - h_dst) / 2;
	info(1, "Resample to %dx%d at %d,%d\n", w_dst, h_dst, x_dst, y_dst);
	gdImageCopyResampled(im2, im1, x_dst, y_dst, 0, 0, w_dst, h_dst, w_src, h_src);
    } else {
	const int x_dst = (fb_w - w_dst) / 2;
	const int y_dst = (fb_h - h_dst) / 2;
	info(1, "Copy to %dx%d at %d,%d\n", w_dst, h_dst, x_dst, y_dst);
	gdImageCopy(im2, im1, w_dst, w_dst, 0, 0, w_src, h_src);
    }
    fb_dump(im2);
    gdImageDestroy(im2);
    gdImageDestroy(im1);

    snprintf(buff, sizeof(buff), "%s", basename(filename));
    suffix = strrchr(buff, '.');
    if (suffix)
      *suffix = '\0';

    int x1 = 0;
    int y1 = vinfo.yres - font->h;
    fb_puttext(&x1, &y1, 0xffffff, buff);
}

/**
 * @brief print usage information to stderr
 */
void usage(char** argv)
{
    const char* program = basename(argv[0]);
    fprintf(stderr, "Usage: %s [OPTIONS] <imagefile.ext>\n", program);
    fprintf(stderr, "Where [OPTIONS] may be one or more of:\n");
    fprintf(stderr, "-u          Up scale small images to TFT size\n");
    fprintf(stderr, "-v          Be verbose\n");
    fprintf(stderr, "-fb=<dev>   Use frame buffer device <dev> (default %s)\n", DEFAULT_FB);
}

int main(int argc, char** argv)
{
    const char* fbdev = DEFAULT_FB;
    int nfiles = 0;
    int upscale = 0;

    for (int i = 1; i < argc; i++) {
        if (!strncmp("-fb=", argv[i], 4)) {
            fbdev = argv[i] + 4;
            continue;
        }
    	if (!strcmp(argv[i], "-u")) {
            upscale = 1;
            continue;
    	}
    	if (!strcmp(argv[i], "-v")) {
            verbose = 1;
            continue;
    	}
	nfiles++;
    }

    atexit(fb_exit);
    gdSetErrorMethod(gd_error);

    int res = fb_init(fbdev);
    if (res < 0) {
        return 1;
    }
    fb_clear();

    info(1, "Using GD version %s %s\n",
	 gdVersionString(), gdExtraVersion());
    info(1, "Framebuffer '%s' is %dx%d, %dbpp\n",
        fb_devname, fb_w, fb_h, fb_bpp);

    if (nfiles < 1) {
    	test_lines(0);
    	usage(argv);
    	return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-')
            continue;
    	load_image(argv[i], upscale);
    	sleep(1);
    }
    return 0;
}
