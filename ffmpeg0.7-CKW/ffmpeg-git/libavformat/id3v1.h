/*
 * ID3v1 header parser
 * Copyright (c) 2003 Fabrice Bellard
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

#ifndef AVFORMAT_ID3V1_H
#define AVFORMAT_ID3V1_H

#include "avformat.h"

//****************************************************************************//
//libavformat\id3v1.h, libavformat\id3v1.c
//	mp3标签信息id3v1
//学习的地方：
//1.
//附录：
//1.扩展知识----id3v-v1&v2&v3简介.txt
//****************************************************************************//


#define ID3v1_TAG_SIZE 128

#define ID3v1_GENRE_MAX 147

/**
 * ID3v1 genres
 */
extern const char *const ff_id3v1_genre_str[ID3v1_GENRE_MAX + 1];

/**
 * Read an ID3v1 tag
 */
void ff_id3v1_read(AVFormatContext *s);

#endif /* AVFORMAT_ID3V1_H */

