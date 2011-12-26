/*
 * MLP codec common code
 * Copyright (c) 2007-2008 Ian Caulfield
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

#include "../stdint.h"

#include "libavutil/crc.h"
#include "libavutil/intreadwrite.h"
#include "mlp.h"

const uint8_t ff_mlp_huffman_tables[3][18][2] = {
    {    /* Huffman table 0, -7 - +10 */
        {0x01, 9}, {0x01, 8}, {0x01, 7}, {0x01, 6}, {0x01, 5}, {0x01, 4}, {0x01, 3},
        {0x04, 3}, {0x05, 3}, {0x06, 3}, {0x07, 3},
        {0x03, 3}, {0x05, 4}, {0x09, 5}, {0x11, 6}, {0x21, 7}, {0x41, 8}, {0x81, 9},
    }, { /* Huffman table 1, -7 - +8 */
        {0x01, 9}, {0x01, 8}, {0x01, 7}, {0x01, 6}, {0x01, 5}, {0x01, 4}, {0x01, 3},
        {0x02, 2}, {0x03, 2},
        {0x03, 3}, {0x05, 4}, {0x09, 5}, {0x11, 6}, {0x21, 7}, {0x41, 8}, {0x81, 9},
    }, { /* Huffman table 2, -7 - +7 */
        {0x01, 9}, {0x01, 8}, {0x01, 7}, {0x01, 6}, {0x01, 5}, {0x01, 4}, {0x01, 3},
        {0x01, 1},
        {0x03, 3}, {0x05, 4}, {0x09, 5}, {0x11, 6}, {0x21, 7}, {0x41, 8}, {0x81, 9},
    }
};

static int crc_init = 0;
static AVCRC crc_63[1024];
static AVCRC crc_1D[1024];
static AVCRC crc_2D[1024];

av_cold void ff_mlp_init_crc(void)
{
    if (!crc_init) {
        av_crc_init(crc_63, 0,  8,   0x63, sizeof(crc_63));
        av_crc_init(crc_1D, 0,  8,   0x1D, sizeof(crc_1D));
        av_crc_init(crc_2D, 0, 16, 0x002D, sizeof(crc_2D));
        crc_init = 1;
    }
}

uint16_t ff_mlp_checksum16(const uint8_t *buf, unsigned int buf_size)
{
    uint16_t crc;

    crc = av_crc(crc_2D, 0, buf, buf_size - 2);
    crc ^= AV_RL16(buf + buf_size - 2);
    return crc;
}

uint8_t ff_mlp_checksum8(const uint8_t *buf, unsigned int buf_size)
{
    uint8_t checksum = av_crc(crc_63, 0x3c, buf, buf_size - 1); // crc_63[0xa2] == 0x3c
    checksum ^= buf[buf_size-1];
    return checksum;
}

uint8_t ff_mlp_restart_checksum(const uint8_t *buf, unsigned int bit_size)
{
    int i;
    int num_bytes = (bit_size + 2) / 8;

    int crc = crc_1D[buf[0] & 0x3f];
    crc = av_crc(crc_1D, crc, buf + 1, num_bytes - 2);
    crc ^= buf[num_bytes - 1];

    for (i = 0; i < ((bit_size + 2) & 7); i++) {
        crc <<= 1;
        if (crc & 0x100)
            crc ^= 0x11D;
        crc ^= (buf[num_bytes] >> (7 - i)) & 1;
    }

    return crc;
}

uint8_t ff_mlp_calculate_parity(const uint8_t *buf, unsigned int buf_size)
{
    uint32_t scratch = 0;
    const uint8_t *buf_end = buf + buf_size;

    for (; buf < buf_end - 3; buf += 4)
        scratch ^= *((const uint32_t*)buf);

    scratch = xor_32_to_8(scratch);

    for (; buf < buf_end; buf++)
        scratch ^= *buf;

    return scratch;
}
