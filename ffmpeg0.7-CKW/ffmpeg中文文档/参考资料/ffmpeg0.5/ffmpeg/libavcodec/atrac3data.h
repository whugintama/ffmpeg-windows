/*
 * Atrac 3 compatible decoder data
 * Copyright (c) 2006-2007 Maxim Poliakovski
 * Copyright (c) 2006-2007 Benjamin Larsson
 *
 * This file is part of FFmpeg.
 *
 * FFmpeg is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * FFmpeg is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with FFmpeg; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

/**
 * @file libavcodec/atrac3data.h
 * Atrac 3 AKA RealAudio 8 compatible decoder data
 */

#ifndef AVCODEC_ATRAC3DATA_H
#define AVCODEC_ATRAC3DATA_H

#include "../stdint.h"

/* VLC tables */

static const uint8_t huffcode1[9] = {
  0x0,0x4,0x5,0xC,0xD,0x1C,0x1D,0x1E,0x1F,
};

static const uint8_t huffbits1[9] = {
  1,3,3,4,4,5,5,5,5,
};

static const uint8_t huffcode2[5] = {
  0x0,0x4,0x5,0x6,0x7,
};

static const uint8_t huffbits2[5] = {
  1,3,3,3,3,
};

static const uint8_t huffcode3[7] = {
0x0,0x4,0x5,0xC,0xD,0xE,0xF,
};

static const uint8_t huffbits3[7] = {
  1,3,3,4,4,4,4,
};

static const uint8_t huffcode4[9] = {
  0x0,0x4,0x5,0xC,0xD,0x1C,0x1D,0x1E,0x1F,
};

static const uint8_t huffbits4[9] = {
  1,3,3,4,4,5,5,5,5,
};

static const uint8_t huffcode5[15] = {
  0x0,0x2,0x3,0x8,0x9,0xA,0xB,0x1C,0x1D,0x3C,0x3D,0x3E,0x3F,0xC,0xD,
};

static const uint8_t huffbits5[15] = {
  2,3,3,4,4,4,4,5,5,6,6,6,6,4,4
};

static const uint8_t huffcode6[31] = {
  0x0,0x2,0x3,0x4,0x5,0x6,0x7,0x14,0x15,0x16,0x17,0x18,0x19,0x34,0x35,
  0x36,0x37,0x38,0x39,0x3A,0x3B,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,0x8,0x9,
};

static const uint8_t huffbits6[31] = {
  3,4,4,4,4,4,4,5,5,5,5,5,5,6,6,6,6,6,6,6,6,7,7,7,7,7,7,7,7,4,4
};

static const uint8_t huffcode7[63] = {
  0x0,0x8,0x9,0xA,0xB,0xC,0xD,0xE,0xF,0x10,0x11,0x24,0x25,0x26,0x27,0x28,
  0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,0x30,0x31,0x32,0x33,0x68,0x69,0x6A,0x6B,0x6C,
  0x6D,0x6E,0x6F,0x70,0x71,0x72,0x73,0x74,0x75,0xEC,0xED,0xEE,0xEF,0xF0,0xF1,0xF2,
  0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF,0x2,0x3,
};

static const uint8_t huffbits7[63] = {
  3,5,5,5,5,5,5,5,5,5,5,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,6,7,7,7,7,7,
  7,7,7,7,7,7,7,7,7,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,8,4,4
};

static const uint8_t huff_tab_sizes[7] = {
  9, 5, 7, 9, 15, 31, 63,
};

static const uint8_t* const huff_codes[7] = {
  huffcode1,huffcode2,huffcode3,huffcode4,huffcode5,huffcode6,huffcode7,
};

static const uint8_t* const huff_bits[7] = {
  huffbits1,huffbits2,huffbits3,huffbits4,huffbits5,huffbits6,huffbits7,
};

/* selector tables */

static const uint8_t CLCLengthTab[8] = {0, 4, 3, 3, 4, 4, 5, 6};
static const int8_t seTab_0[4] = {0, 1, -2, -1};
static const int8_t decTable1[18] = {0,0, 0,1, 0,-1, 1,0, -1,0, 1,1, 1,-1, -1,1, -1,-1};


/* tables for the scalefactor decoding */

static const float iMaxQuant[8] = {
  0.0, 1.0/1.5, 1.0/2.5, 1.0/3.5, 1.0/4.5, 1.0/7.5, 1.0/15.5, 1.0/31.5
};

static const uint16_t subbandTab[33] = {
  0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224,
  256, 288, 320, 352, 384, 416, 448, 480, 512, 576, 640, 704, 768, 896, 1024
};

/* transform data */

static const float qmf_48tap_half[24] = {
   -0.00001461907, -0.00009205479, -0.000056157569, 0.00030117269,
    0.0002422519,-0.00085293897, -0.0005205574, 0.0020340169,
    0.00078333891, -0.0042153862, -0.00075614988, 0.0078402944,
   -0.000061169922, -0.01344162, 0.0024626821, 0.021736089,
   -0.007801671, -0.034090221, 0.01880949, 0.054326009,
   -0.043596379, -0.099384367, 0.13207909, 0.46424159
};

/* joint stereo related tables */
static const float matrixCoeffs[8] = {0.0, 2.0, 2.0, 2.0, 0.0, 0.0, 1.0, 1.0};

#endif /* AVCODEC_ATRAC3DATA_H */
