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

#define	DEFAULT_FB  "/dev/fb1"

/**
 * @brief Shift direction for @ref fb_shift()
 */
typedef enum {
    shift_left,
    shift_right,
    shift_up,
    shift_down
}   shift_dir_e;

extern int fb_init(struct sfb_s** sfb, const char* devname);
extern void fb_exit(struct sfb_s** sfb);

extern const char* fb_devname(struct sfb_s* sfb);
extern int fb_w(struct sfb_s* sfb);
extern int fb_h(struct sfb_s* sfb);
extern int fb_bpp(struct sfb_s* sfb);

extern void fb_clear(struct sfb_s* sfb);
extern void fb_line(struct sfb_s* sfb, int x1, int y1, int x2, int y2, unsigned color);
extern void fb_shift(struct sfb_s* sfb, shift_dir_e dir, int pixels);
extern void fb_putchar(struct sfb_s* sfb, unsigned color, char ch);
extern void fb_puttext(struct sfb_s* sfb, unsigned color, const char* text);
extern size_t fb_printf(struct sfb_s* sfb, const char* format, ...);
extern void fb_dump(struct sfb_s* sfb, gdImagePtr im);
