/*********************************************************
 * font.c - Fixed 6x8 font
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
#include "font.h"

static const unsigned char font_6x8_data[] = {
  0x00,0x1f,0x11,0x11,0x11,0x11,0x11,0x1f,  /* 00 NUL */
  0x00,0x1f,0x10,0x10,0x10,0x10,0x10,0x10,  /* 01 SOH */
  0x00,0x04,0x04,0x04,0x04,0x04,0x04,0x1f,  /* 02 STX */
  0x00,0x01,0x01,0x01,0x01,0x01,0x01,0x1f,  /* 03 */
  0x00,0x08,0x04,0x02,0x0f,0x04,0x02,0x01,  /* 04 */
  0x00,0x1f,0x11,0x1b,0x15,0x1b,0x11,0x1f,  /* 05 */
  0x00,0x00,0x01,0x02,0x14,0x18,0x10,0x00,  /* 06 */
  0x00,0x0e,0x11,0x11,0x1f,0x0a,0x0a,0x1b,  /* 07 */
  0x00,0x04,0x08,0x1e,0x09,0x05,0x01,0x01,  /* 08 */
  0x00,0x00,0x04,0x02,0x1f,0x02,0x04,0x00,  /* 09 */
  0x00,0x1f,0x00,0x00,0x1f,0x00,0x00,0x1f,  /* 0a */
  0x00,0x00,0x04,0x04,0x15,0x0e,0x04,0x00,  /* 0b */
  0x00,0x04,0x15,0x0e,0x04,0x15,0x0e,0x04,  /* 0c */
  0x00,0x00,0x04,0x08,0x1f,0x08,0x04,0x00,  /* 0d */
  0x00,0x0e,0x11,0x1b,0x15,0x1b,0x11,0x0e,  /* 0e */
  0x00,0x0e,0x11,0x11,0x15,0x11,0x11,0x0e,  /* 0f */
  0x00,0x1f,0x11,0x11,0x1f,0x11,0x11,0x1f,  /* 10 */
  0x00,0x0e,0x15,0x15,0x17,0x11,0x11,0x0e,  /* 11 */
  0x00,0x0e,0x11,0x11,0x17,0x15,0x15,0x0e,  /* 12 */
  0x00,0x0e,0x11,0x11,0x1d,0x15,0x15,0x0e,  /* 13 */
  0x00,0x0e,0x15,0x15,0x1d,0x11,0x11,0x0e,  /* 14 */
  0x00,0x00,0x05,0x02,0x15,0x18,0x10,0x00,  /* 15 */
  0x00,0x0e,0x0a,0x0a,0x0a,0x0a,0x0a,0x1b,  /* 16 */
  0x00,0x01,0x01,0x01,0x1f,0x01,0x01,0x01,  /* 17 */
  0x00,0x1f,0x11,0x0a,0x04,0x0a,0x11,0x1f,  /* 18 */
  0x00,0x04,0x04,0x0e,0x0e,0x04,0x04,0x04,  /* 19 */
  0x00,0x0e,0x11,0x10,0x08,0x04,0x00,0x04,  /* 1a */
  0x00,0x0e,0x11,0x11,0x1f,0x11,0x11,0x0e,  /* 1b */
  0x00,0x1f,0x15,0x15,0x1d,0x11,0x11,0x1f,  /* 1c */
  0x00,0x1f,0x11,0x11,0x1d,0x15,0x15,0x1f,  /* 1d */
  0x00,0x1f,0x11,0x11,0x17,0x15,0x15,0x1f,  /* 1e */
  0x00,0x1f,0x15,0x15,0x17,0x11,0x11,0x1f,  /* 1f */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,  /* 20 space */
  0x00,0x04,0x04,0x04,0x04,0x04,0x00,0x04,  /* 21 exclam (!) */
  0x00,0x0a,0x0a,0x0a,0x00,0x00,0x00,0x00,  /* 22 quote (") */
  0x00,0x0a,0x0a,0x1f,0x0a,0x1f,0x0a,0x0a,  /* 23 number (#) */
  0x00,0x04,0x0f,0x14,0x0e,0x05,0x1e,0x04,  /* 24 dollar ($) */
  0x00,0x18,0x19,0x02,0x04,0x08,0x13,0x03,  /* 25 percent (%) */
  0x00,0x08,0x14,0x14,0x08,0x15,0x12,0x0d,  /* 26 ampersand (&) */
  0x00,0x0c,0x0c,0x08,0x10,0x00,0x00,0x00,  /* 27 apostrophe (') */
  0x00,0x02,0x04,0x08,0x08,0x08,0x04,0x02,  /* 28 lparen [(] */
  0x00,0x08,0x04,0x02,0x02,0x02,0x04,0x08,  /* 29 rparen [)] */
  0x00,0x04,0x15,0x0e,0x1f,0x0e,0x15,0x04,  /* 2a asterisk (*) */
  0x00,0x00,0x04,0x04,0x1f,0x04,0x04,0x00,  /* 2b plus (+) */
  0x00,0x00,0x00,0x00,0x0c,0x0c,0x08,0x10,  /* 2c comma (,) */
  0x00,0x00,0x00,0x00,0x1f,0x00,0x00,0x00,  /* 2d minus (-) */
  0x00,0x00,0x00,0x00,0x00,0x00,0x0c,0x0c,  /* 2e period (.) */
  0x00,0x00,0x01,0x02,0x04,0x08,0x10,0x00,  /* 2f slash (/) */
  0x00,0x0e,0x11,0x13,0x15,0x19,0x11,0x0e,  /* 30 zero (0) */
  0x00,0x04,0x0c,0x04,0x04,0x04,0x04,0x0e,  /* 31 one (1) */
  0x00,0x0e,0x11,0x01,0x0e,0x10,0x10,0x1f,  /* 32 two (2) */
  0x00,0x0e,0x11,0x01,0x06,0x01,0x11,0x0e,  /* 33 three (3) */
  0x00,0x02,0x06,0x0a,0x12,0x1f,0x02,0x02,  /* 34 four (4) */
  0x00,0x1f,0x10,0x1e,0x01,0x01,0x11,0x0e,  /* 35 five (5) */
  0x00,0x06,0x08,0x10,0x1e,0x11,0x11,0x0e,  /* 36 six (6) */
  0x00,0x1f,0x01,0x02,0x04,0x08,0x10,0x10,  /* 37 seven (7) */
  0x00,0x0e,0x11,0x11,0x0e,0x11,0x11,0x0e,  /* 38 eight (8) */
  0x00,0x0e,0x11,0x11,0x0f,0x01,0x02,0x0c,  /* 39 nine (9) */
  0x00,0x00,0x0c,0x0c,0x00,0x0c,0x0c,0x00,  /* 3a colon (:) */
  0x00,0x0c,0x0c,0x00,0x0c,0x0c,0x08,0x10,  /* 3b semicolon (;) */
  0x00,0x02,0x04,0x08,0x10,0x08,0x04,0x02,  /* 3c less (<) */
  0x00,0x00,0x00,0x1f,0x00,0x1f,0x00,0x00,  /* 3d equal (=) */
  0x00,0x08,0x04,0x02,0x01,0x02,0x04,0x08,  /* 3e greater (>) */
  0x00,0x0e,0x11,0x01,0x02,0x04,0x00,0x04,  /* 3f question (?) */
  0x00,0x0e,0x11,0x01,0x0d,0x15,0x15,0x0e,  /* 40 at (@) */
  0x00,0x04,0x0a,0x11,0x11,0x1f,0x11,0x11,  /* 41 upper case A */
  0x00,0x1e,0x09,0x09,0x0e,0x09,0x09,0x1e,  /* 42 upper case B */
  0x00,0x0e,0x11,0x10,0x10,0x10,0x11,0x0e,  /* 43 upper case C */
  0x00,0x1e,0x09,0x09,0x09,0x09,0x09,0x1e,  /* 44 upper case D */
  0x00,0x1f,0x10,0x10,0x1c,0x10,0x10,0x1f,  /* 45 upper case E */
  0x00,0x1f,0x10,0x10,0x1c,0x10,0x10,0x10,  /* 46 upper case F */
  0x00,0x0f,0x10,0x10,0x13,0x11,0x11,0x0f,  /* 47 upper case G */
  0x00,0x11,0x11,0x11,0x1f,0x11,0x11,0x11,  /* 48 upper case H */
  0x00,0x0e,0x04,0x04,0x04,0x04,0x04,0x0e,  /* 49 upper case I */
  0x00,0x01,0x01,0x01,0x01,0x01,0x11,0x0e,  /* 4a upper case J */
  0x00,0x11,0x12,0x14,0x18,0x14,0x12,0x11,  /* 4b upper case K */
  0x00,0x10,0x10,0x10,0x10,0x10,0x10,0x1f,  /* 4c upper case L */
  0x00,0x11,0x1b,0x15,0x15,0x11,0x11,0x11,  /* 4d upper case M */
  0x00,0x11,0x19,0x15,0x13,0x11,0x11,0x11,  /* 4e upper case N */
  0x00,0x0e,0x11,0x11,0x11,0x11,0x11,0x0e,  /* 4f upper case O */
  0x00,0x1e,0x11,0x11,0x1e,0x10,0x10,0x10,  /* 50 upper case P */
  0x00,0x0e,0x11,0x11,0x11,0x15,0x12,0x0d,  /* 51 upper case Q */
  0x00,0x1e,0x11,0x11,0x1e,0x14,0x12,0x11,  /* 52 upper case R */
  0x00,0x0e,0x11,0x10,0x0e,0x01,0x11,0x0e,  /* 53 upper case S */
  0x00,0x1f,0x04,0x04,0x04,0x04,0x04,0x04,  /* 54 upper case T */
  0x00,0x11,0x11,0x11,0x11,0x11,0x11,0x0e,  /* 55 upper case U */
  0x00,0x11,0x11,0x11,0x0a,0x0a,0x04,0x04,  /* 56 upper case V */
  0x00,0x11,0x11,0x11,0x11,0x15,0x1b,0x11,  /* 57 upper case W */
  0x00,0x11,0x11,0x0a,0x04,0x0a,0x11,0x11,  /* 58 upper case X */
  0x00,0x11,0x11,0x0a,0x04,0x04,0x04,0x04,  /* 59 upper case Y */
  0x00,0x1f,0x01,0x02,0x04,0x08,0x10,0x1f,  /* 5a upper case Z */
  0x00,0x04,0x0e,0x15,0x04,0x04,0x04,0x04,  /* 5b left square bracket ([) */
  0x00,0x04,0x04,0x04,0x04,0x15,0x0e,0x04,  /* 5c backslash (\) */
  0x00,0x00,0x04,0x08,0x1f,0x08,0x04,0x00,  /* 5d right square bracket (]) */
  0x00,0x00,0x04,0x02,0x1f,0x02,0x04,0x00,  /* 5e circumflex (^) */
  0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x1f,  /* 5f underscore (_) */
  0x00,0x06,0x06,0x04,0x02,0x00,0x00,0x00,  /* 60 grave (`) */
  0x00,0x00,0x00,0x0e,0x01,0x0f,0x11,0x0f,  /* 61 lower case a */
  0x00,0x10,0x10,0x16,0x19,0x11,0x19,0x16,  /* 62 lower case b */
  0x00,0x00,0x00,0x0e,0x11,0x10,0x11,0x0e,  /* 63 lower case c */
  0x00,0x01,0x01,0x0d,0x13,0x11,0x13,0x0d,  /* 64 lower case d */
  0x00,0x00,0x00,0x0e,0x11,0x1f,0x10,0x0e,  /* 65 lower case e */
  0x00,0x02,0x05,0x04,0x0e,0x04,0x04,0x04,  /* 66 lower case f */
  0x00,0x0d,0x13,0x13,0x0d,0x01,0x11,0x0e,  /* 67 lower case g */
  0x00,0x10,0x10,0x16,0x19,0x11,0x11,0x11,  /* 68 lower case h */
  0x00,0x04,0x00,0x0c,0x04,0x04,0x04,0x0e,  /* 69 lower case i */
  0x00,0x01,0x00,0x01,0x01,0x01,0x11,0x0e,  /* 6a lower case j */
  0x00,0x10,0x10,0x12,0x14,0x18,0x14,0x12,  /* 6b lower case k */
  0x00,0x0c,0x04,0x04,0x04,0x04,0x04,0x0e,  /* 6c lower case l */
  0x00,0x00,0x00,0x1a,0x15,0x15,0x15,0x15,  /* 6d lower case m */
  0x00,0x00,0x00,0x16,0x19,0x11,0x11,0x11,  /* 6e lower case n */
  0x00,0x00,0x00,0x0e,0x11,0x11,0x11,0x0e,  /* 6f lower case o */
  0x00,0x16,0x19,0x11,0x19,0x16,0x10,0x10,  /* 70 lower case p */
  0x00,0x0d,0x13,0x11,0x13,0x0d,0x01,0x01,  /* 71 lower case q */
  0x00,0x00,0x00,0x16,0x19,0x10,0x10,0x10,  /* 72 lower case r */
  0x00,0x00,0x00,0x0f,0x10,0x0e,0x01,0x1e,  /* 73 lower case s */
  0x00,0x04,0x04,0x1f,0x04,0x04,0x05,0x02,  /* 74 lower case t */
  0x00,0x00,0x00,0x11,0x11,0x11,0x13,0x0d,  /* 75 lower case u */
  0x00,0x00,0x00,0x11,0x11,0x11,0x0a,0x04,  /* 76 lower case v */
  0x00,0x00,0x00,0x11,0x11,0x15,0x15,0x0a,  /* 77 lower case w */
  0x00,0x00,0x00,0x11,0x0a,0x04,0x0a,0x11,  /* 78 lower case x */
  0x00,0x11,0x11,0x11,0x0f,0x01,0x11,0x0e,  /* 79 lower case y */
  0x00,0x00,0x00,0x1f,0x02,0x04,0x08,0x1f,  /* 7a lower case z */
  0x00,0x02,0x04,0x04,0x08,0x04,0x04,0x02,  /* 7b left curly brace ({) */
  0x00,0x04,0x04,0x04,0x00,0x04,0x04,0x04,  /* 7c vertical bar (|) */
  0x00,0x08,0x04,0x04,0x02,0x04,0x04,0x08,  /* 7d right curly brace (}) */
  0x00,0x08,0x15,0x02,0x00,0x00,0x00,0x00,  /* 7e tilde (~) */
  0x00,0x0a,0x15,0x0a,0x15,0x0a,0x15,0x0a,  /* 7f delete (pattern) */
};

const fbfont_t font_6x8 = { 6, 10, ",.gpqy", 6, 8, font_6x8_data };
