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

#if !defined(DEFAULT_FBDEV)
#define	DEFAULT_FBDEV  "/dev/fb1"
#endif

#if defined(HAVE_STDLIB_H)
#include <stdlib.h>
#endif
#if defined(HAVE_STDIO_H)
#include <stdio.h>
#endif
#if defined(HAVE_STRING_H)
#include <string.h>
#endif
#if defined(HAVE_STDINT_H)
#include <stdint.h>
#endif
#if defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif
#if defined(HAVE_TIME_H)
#include <time.h>
#endif
#if defined(HAVE_LOCALE_H)
#include <locale.h>
#endif
#if defined(HAVE_GETOPT_H)
#include <getopt.h>
#endif

#if defined(HAVE_GD_H)
#include <gd.h>
#endif

#include "sfb.h"

static int verbose = 0;
static struct sfb_s* sfb;

static void info(int priority, const char* format, ...)
{
    va_list ap;

    if (priority > verbose)
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
 * @brief A simple test drawing frames and rectangles across the TFT
 * @param sfb pointer to the libsfb context
 * @param us number of microseconds to delay between drawing
 */
void test_rects(struct sfb_s* sfb, int us)
{
    for (int n = 0; n < 5000; n++) {
	const int x1 = rand() % fb_w(sfb);
	const int y1 = rand() % fb_h(sfb);
	const int w = rand() % 256;
	const int h = rand() % 256;
	const uint32_t color = rand() & 0x00ffffff;
	fb_set_fgcolor(sfb, color);
	if (rand() & 0x4000)
	    fb_fill(sfb, x1, y1, x1 + w - 1, y1 + h - 1);
	else
	    fb_rect(sfb, x1, y1, x1 + w - 1, y1 + h - 1);
	if (us) {
	    usleep(us);
	}
    }
}

/**
 * @brief A simple test drawing lines across the TFT
 * @param sfb pointer to the libsfb context
 * @param us number of microseconds to delay between drawing
 */
void test_lines(struct sfb_s* sfb, int us)
{
    fb_set_fgcolor(sfb, color_White);
    for (int x = 0; x < fb_w(sfb); x += 5) {
	fb_line(sfb, x, 0, fb_w(sfb) - 1 - x, fb_h(sfb) - 1);
	if (us) {
	    usleep(us);
	}
    }

    for (int y = 0; y < fb_h(sfb); y += 5) {
	fb_line(sfb, 0, y, fb_w(sfb) - 1, fb_h(sfb) - 1 - y);
	if (us) {
	    usleep(us);
	}
    }
}

/**
 * @brief A simple test drawing circles and discs across the TFT
 * @param sfb pointer to the libsfb context
 * @param us number of microseconds to delay between drawing
 */
void test_circles(struct sfb_s* sfb, int us)
{
    for (int n = 0; n < 5000; n++) {
	const int x = rand() % fb_w(sfb);
	const int y = rand() % fb_h(sfb);
	const int r = rand() % 64;
        const unsigned char oct = ((rand() & 0xff) == 0x55) ? rand() & 255 : 255;
	const uint32_t color = rand() & 0x00ffffff;
	fb_set_fgcolor(sfb, color);
	if (rand() & 0x4000)
	    fb_disc_octants(sfb, oct, x, y, r);
	else
	    fb_circle_octants(sfb, oct, x, y, r);
	if (us) {
	    usleep(us);
	}
    }
}

/**
 * @brief A simple test writing a text to the TFT display
 * @param sfb pointer to the libsfb context
 * @param us number of microseconds to delay between drawing
 */
void test_text(struct sfb_s* sfb, int us)
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
	    "The pangs of dispised Love, the Law's delay,\n"
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
	    "Be all my sins remember'd.\n"
	    "\n"
	    "A quick brown fox jumps over the lazy dog.\n"
	    "Falsches Üben von Xylophonmusik quält jeden größeren Zwerg.\n"
	    "Dès Noël où un zéphyr haï me vêt de glaçons würmiens,\n"
	    "je dîne d’exquis rôtis de bœuf au kir à l’aÿ d’âge mûr & cætera !\n";

    fb_set_fgcolor(sfb, color_White);
    for (const char* line = text; *line; line = strchr(line, '\n') + 1) {
        char buff[256];
	const char* eol = strchr(line, '\n') ? strchr(line, '\n') : line + strlen(line);
	const size_t len = (size_t)(eol + 1 - line);
	snprintf(buff, sizeof(buff), "%.*s", (int)len, line);
	fb_puts(sfb, buff);
	if (us) {
	    usleep(us);
	}
    }
}

/**
 * @brief Load an image file and display it
 * @param sfb pointer to the libsfb context
 * @param filename of the image
 * @param upscale if non zero, scale smaller images up to the TFT dimensions
 */
void load_image(struct sfb_s* sfb, const char* filename, int upscale)
{
    char buff[128], *suffix;
    gdImagePtr im1, im2;

    fb_set_font(sfb, Font_8x13);
    fb_set_bgcolor(sfb, color_Black);
    fb_clear(sfb);
    fb_set_fgcolor(sfb, color_Light_Yellow);
    fb_set_opaque(sfb, 0); 

    im1 = gdImageCreateFromFile(filename);
    if (NULL == im1) {
	printf("Loading %s failed\n", filename);
	return;
    }

    const int w_src = gdImageSX(im1);
    const int h_src = gdImageSY(im1);
    info(1, "Loaded image '%s' %dx%d %dbpp\n", filename, w_src, h_src, gdImageTrueColor(im1) ? 32 : 8);

    im2 = gdImageCreateTrueColor(fb_w(sfb), fb_h(sfb));
    int w_dst = w_src;
    int h_dst = h_src;
    if (upscale || w_src > fb_w(sfb) || h_src > fb_h(sfb)) {
	if (fb_w(sfb) > fb_h(sfb)) {
	    w_dst = fb_w(sfb);
	    h_dst = h_src * fb_w(sfb) / w_src;
	    if (h_dst > fb_h(sfb)) {
		h_dst = fb_h(sfb);
		w_dst = w_src * fb_h(sfb) / h_src;
	    }
	} else {
	    h_dst = fb_h(sfb);
	    w_dst = w_src * fb_w(sfb) / h_src;
	    if (w_dst > fb_w(sfb)) {
		w_dst = fb_w(sfb);
		h_dst = h_src * fb_w(sfb) / w_src;
	    }
	}
	const int x_dst = (fb_w(sfb) - w_dst) / 2;
	const int y_dst = (fb_h(sfb) - h_dst) / 2;
	info(1, "Resample to %dx%d at %d,%d\n", w_dst, h_dst, x_dst, y_dst);
	gdImageCopyResampled(im2, im1, x_dst, y_dst, 0, 0, w_dst, h_dst, w_src, h_src);
    } else {
	const int x_dst = (fb_w(sfb) - w_dst) / 2;
	const int y_dst = (fb_h(sfb) - h_dst) / 2;
	info(1, "Copy to %dx%d at %d,%d\n", w_dst, h_dst, x_dst, y_dst);
	gdImageCopy(im2, im1, x_dst, y_dst, 0, 0, w_src, h_src);
    }
    fb_dump(sfb, im2);
    gdImageDestroy(im2);
    gdImageDestroy(im1);

    snprintf(buff, sizeof(buff), "%s", basename(filename));
    suffix = strrchr(buff, '.');
    if (suffix)
	*suffix = '\0';

    fb_gotoxy(sfb, 0, fb_h(sfb) - fb_font_h(sfb));
    fb_puts(sfb, buff);
}

static const struct option longopts[] = {
    { "fbdevice", required_argument,  NULL, 'f' },
    { "help",     no_argument,        NULL, 'h' },
    { "upscale",  no_argument,        NULL, 'u' },
    { "verbose",  no_argument,        NULL, 'v' },
    { "version",  no_argument,        NULL, 'V' },
    { NULL, 0, NULL, 0 }
};

/**
 * @brief print usage information to stderr
 */
void usage(const char* program)
{
    fprintf(stderr, "Usage: %s [OPTIONS] <imagefile.ext>\n", program);
    fprintf(stderr, "Where [OPTIONS] may be one or more of:\n");
    fprintf(stderr, "-f, --fbdevice <dev>  Use frame buffer device <dev> (default %s)\n", DEFAULT_FBDEV);
    fprintf(stderr, "-h, --help            Print this help\n");
    fprintf(stderr, "-u, --upscale         Up scale small images to TFT size\n");
    fprintf(stderr, "-v, --verbose         Be verbose\n");
    fprintf(stderr, "-V, --version         Print %s version\n", program);
}

int main(int argc, char** argv)
{
    const char* program = basename(argv[0]);
    const char* fbdev = DEFAULT_FBDEV;
    int nfiles = 0;
    int upscale = 0;
    int us = 700;

    setlocale(LC_ALL, "C.UTF-8");
    srand(time(NULL));

    int c;
    while ((c = getopt_long(argc, argv, "f:uvV", longopts, NULL)) != -1) {
    switch (c) {
        case 'f':
                fbdev = optarg;
                break;
	case 'u':
		upscale = 1;
		break;
	case 'v':
		verbose++;
		break;
	case 'V':
		printf("%s\n", VERSION);
		exit(EXIT_SUCCESS);
	case '?':
	case 'h':
	default:
		usage(program);
	}
    }
    for (int i = optind; i < argc; i++) {
	nfiles++;
    }

    gdSetErrorMethod(gd_error);

    int res = fb_init(&sfb, fbdev);
    if (res < 0) {
	return 1;
    }

    info(1, "Using GD version %s %s\n",
	 gdVersionString(), gdExtraVersion());
    info(1, "Framebuffer '%s' is %dx%d, %dbpp\n",
	 fb_devname(sfb), fb_w(sfb), fb_h(sfb), fb_bpp(sfb));

    fb_clear(sfb);
    if (nfiles < 1) {
	test_rects(sfb, us);
	test_lines(sfb, us);
	test_text(sfb, us);
	test_circles(sfb, us);
	usage(program);
	return 1;
    }

    for (int i = optind; i < argc; i++) {
	if (argv[i][0] == '-')
	    continue;
	load_image(sfb, argv[i], upscale);
	sleep(1);
    }
    return 0;
}
