/*
 * Copyright (c) 2009 Michael Niedermayer
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

#include "avformat.h"


static int probe(AVProbeData *p)
{
    // the single file i have starts with that, i dont know if others do too
    if(   p->buf[0] == 1
            && p->buf[1] == 1
            && p->buf[2] == 3
            && p->buf[3] == 0xB8
            && p->buf[4] == 0x80
            && p->buf[5] == 0x60
      )
        return AVPROBE_SCORE_MAX - 2;

    return 0;
}

static int read_header(AVFormatContext *s, AVFormatParameters *ap)
{
    AVStream *st;

    st = av_new_stream(s, 0);
    if (!st)
        return AVERROR(ENOMEM);

    st->codec->codec_type = AVMEDIA_TYPE_VIDEO;
    st->codec->codec_id = CODEC_ID_MPEG4;
    st->need_parsing = AVSTREAM_PARSE_FULL;
    av_set_pts_info(st, 64, 1, 90000);

    return 0;

}

static int read_packet(AVFormatContext *s, AVPacket *pkt)
{
    int ret, size, pts, type;
retry:
    type = avio_rb16(s->pb); // 257 or 258
    size = avio_rb16(s->pb);

    avio_rb16(s->pb); //some flags, 0x80 indicates end of frame
    avio_rb16(s->pb); //packet number
    pts = avio_rb32(s->pb);
    avio_rb32(s->pb); //6A 13 E3 88

    size -= 12;
    if(size < 1)
        return -1;

    if(type == 258)
    {
        avio_skip(s->pb, size);
        goto retry;
    }

    ret = av_get_packet(s->pb, pkt, size);

    pkt->pts = pts;
    pkt->pos -= 16;

    pkt->stream_index = 0;

    return ret;
}

AVInputFormat ff_iv8_demuxer =
{
    "iv8",
    NULL_IF_CONFIG_SMALL("A format generated by IndigoVision 8000 video server"),
    0,
    probe,
    read_header,
    read_packet,
    .flags = AVFMT_GENERIC_INDEX,
    .value = CODEC_ID_MPEG4,
};
