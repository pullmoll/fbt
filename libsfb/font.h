/*********************************************************
 * font.h - Fixed 6x8 font
 * Coypright Jürgen Buchmüller <pullmoll@t-online.de>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 * notice, this list of conditions and the following disclaimer in the
 * documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ****************************************************************************/
#include <stdlib.h>

/**
 * @brief the frame buffer font structure contains information
 * and pointers to data for a bitmapped font.
 * The width @p w and height @p h should be obvious.
 *
 * The @p map contains information about the glyphs which
 * are defined in a font. This is usually a (subset of)
 * the ISO-8859-1 glyphs.
 *
 * Finally there is a pointer to the bitmap data. Depending
 * on the width of the font, this is either bytes (uint8_t),
 * words (uint16_t) or dwords (uint32_t).
 *
 * There is one element per glyph row (height).
 */
typedef struct fbfont_s {
    int w;                      /*!< font cell width */
    int h;                      /*!< font cell height */
    const wchar_t* map;         /*!< map of wide characters (unicode) */
    const void* data;           /*!< font definition bitmaps */
}   fbfont_t;

extern const fbfont_t font_6x12;
extern const fbfont_t font_8x13;
extern const fbfont_t font_9x15;
extern const fbfont_t font_10x20;
