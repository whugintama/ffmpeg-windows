/*
 * Sierra VMD Audio & Video Decoders
 * Copyright (C) 2004 the ffmpeg project
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
 * @file
 * Sierra VMD audio & video decoders
 * by Vladimir "VAG" Gneushev (vagsoft at mail.ru)
 * for more information on the Sierra VMD format, visit:
 *   http://www.pcisys.net/~melanson/codecs/
 *
 * The video decoder outputs PAL8 colorspace data. The decoder expects
 * a 0x330-byte VMD file header to be transmitted via extradata during
 * codec initialization. Each encoded frame that is sent to this decoder
 * is expected to be prepended with the appropriate 16-byte frame
 * information record from the VMD file.
 *
 * The audio decoder, like the video decoder, expects each encoded data
 * chunk to be prepended with the appropriate 16-byte frame information
 * record from the VMD file. It does not require the 0x330-byte VMD file
 * header, but it does need the audio setup parameters passed in through
 * normal libavcodec API means.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libavutil/intreadwrite.h"
#include "avcodec.h"

#define VMD_HEADER_SIZE 0x330
#define PALETTE_COUNT 256

/*
 * Video Decoder
 */

typedef struct VmdVideoContext
{

    AVCodecContext *avctx;
    AVFrame frame;
    AVFrame prev_frame;

    const unsigned char *buf;
    int size;

    unsigned char palette[PALETTE_COUNT * 4];
    unsigned char *unpack_buffer;
    int unpack_buffer_size;

    int x_off, y_off;
} VmdVideoContext;

#define QUEUE_SIZE 0x1000
#define QUEUE_MASK 0x0FFF

static void lz_unpack(const unsigned char *src, unsigned char *dest, int dest_len)
{
    const unsigned char *s;
    unsigned char *d;
    unsigned char *d_end;
    unsigned char queue[QUEUE_SIZE];
    unsigned int qpos;
    unsigned int dataleft;
    unsigned int chainofs;
    unsigned int chainlen;
    unsigned int speclen;
    unsigned char tag;
    unsigned int i, j;

    s = src;
    d = dest;
    d_end = d + dest_len;
    dataleft = AV_RL32(s);
    s += 4;
    memset(queue, 0x20, QUEUE_SIZE);
    if (AV_RL32(s) == 0x56781234)
    {
        s += 4;
        qpos = 0x111;
        speclen = 0xF + 3;
    }
    else
    {
        qpos = 0xFEE;
        speclen = 100;  /* no speclen */
    }

    while (dataleft > 0)
    {
        tag = *s++;
        if ((tag == 0xFF) && (dataleft > 8))
        {
            if (d + 8 > d_end)
                return;
            for (i = 0; i < 8; i++)
            {
                queue[qpos++] = *d++ = *s++;
                qpos &= QUEUE_MASK;
            }
            dataleft -= 8;
        }
        else
        {
            for (i = 0; i < 8; i++)
            {
                if (dataleft == 0)
                    break;
                if (tag & 0x01)
                {
                    if (d + 1 > d_end)
                        return;
                    queue[qpos++] = *d++ = *s++;
                    qpos &= QUEUE_MASK;
                    dataleft--;
                }
                else
                {
                    chainofs = *s++;
                    chainofs |= ((*s & 0xF0) << 4);
                    chainlen = (*s++ & 0x0F) + 3;
                    if (chainlen == speclen)
                        chainlen = *s++ + 0xF + 3;
                    if (d + chainlen > d_end)
                        return;
                    for (j = 0; j < chainlen; j++)
                    {
                        *d = queue[chainofs++ & QUEUE_MASK];
                        queue[qpos++] = *d++;
                        qpos &= QUEUE_MASK;
                    }
                    dataleft -= chainlen;
                }
                tag >>= 1;
            }
        }
    }
}

static int rle_unpack(const unsigned char *src, unsigned char *dest,
                      int src_len, int dest_len)
{
    const unsigned char *ps;
    unsigned char *pd;
    int i, l;
    unsigned char *dest_end = dest + dest_len;

    ps = src;
    pd = dest;
    if (src_len & 1)
        *pd++ = *ps++;

    src_len >>= 1;
    i = 0;
    do
    {
        l = *ps++;
        if (l & 0x80)
        {
            l = (l & 0x7F) * 2;
            if (pd + l > dest_end)
                return ps - src;
            memcpy(pd, ps, l);
            ps += l;
            pd += l;
        }
        else
        {
            if (pd + i > dest_end)
                return ps - src;
            for (i = 0; i < l; i++)
            {
                *pd++ = ps[0];
                *pd++ = ps[1];
            }
            ps += 2;
        }
        i += l;
    }
    while (i < src_len);

    return ps - src;
}

static void vmd_decode(VmdVideoContext *s)
{
    int i;
    unsigned int *palette32;
    unsigned char r, g, b;

    /* point to the start of the encoded data */
    const unsigned char *p = s->buf + 16;

    const unsigned char *pb;
    unsigned char meth;
    unsigned char *dp;   /* pointer to current frame */
    unsigned char *pp;   /* pointer to previous frame */
    unsigned char len;
    int ofs;

    int frame_x, frame_y;
    int frame_width, frame_height;
    int dp_size;

    frame_x = AV_RL16(&s->buf[6]);
    frame_y = AV_RL16(&s->buf[8]);
    frame_width = AV_RL16(&s->buf[10]) - frame_x + 1;
    frame_height = AV_RL16(&s->buf[12]) - frame_y + 1;

    if ((frame_width == s->avctx->width && frame_height == s->avctx->height) &&
            (frame_x || frame_y))
    {

        s->x_off = frame_x;
        s->y_off = frame_y;
    }
    frame_x -= s->x_off;
    frame_y -= s->y_off;

    /* if only a certain region will be updated, copy the entire previous
     * frame before the decode */
    if (frame_x || frame_y || (frame_width != s->avctx->width) ||
            (frame_height != s->avctx->height))
    {

        memcpy(s->frame.data[0], s->prev_frame.data[0],
               s->avctx->height * s->frame.linesize[0]);
    }

    /* check if there is a new palette */
    if (s->buf[15] & 0x02)
    {
        p += 2;
        palette32 = (unsigned int *)s->palette;
        for (i = 0; i < PALETTE_COUNT; i++)
        {
            r = *p++ * 4;
            g = *p++ * 4;
            b = *p++ * 4;
            palette32[i] = (r << 16) | (g << 8) | (b);
        }
        s->size -= (256 * 3 + 2);
    }
    if (s->size >= 0)
    {
        /* originally UnpackFrame in VAG's code */
        pb = p;
        meth = *pb++;
        if (meth & 0x80)
        {
            lz_unpack(pb, s->unpack_buffer, s->unpack_buffer_size);
            meth &= 0x7F;
            pb = s->unpack_buffer;
        }

        dp = &s->frame.data[0][frame_y * s->frame.linesize[0] + frame_x];
        dp_size = s->frame.linesize[0] * s->avctx->height;
        pp = &s->prev_frame.data[0][frame_y * s->prev_frame.linesize[0] + frame_x];
        switch (meth)
        {
        case 1:
            for (i = 0; i < frame_height; i++)
            {
                ofs = 0;
                do
                {
                    len = *pb++;
                    if (len & 0x80)
                    {
                        len = (len & 0x7F) + 1;
                        if (ofs + len > frame_width)
                            return;
                        memcpy(&dp[ofs], pb, len);
                        pb += len;
                        ofs += len;
                    }
                    else
                    {
                        /* interframe pixel copy */
                        if (ofs + len + 1 > frame_width)
                            return;
                        memcpy(&dp[ofs], &pp[ofs], len + 1);
                        ofs += len + 1;
                    }
                }
                while (ofs < frame_width);
                if (ofs > frame_width)
                {
                    av_log(s->avctx, AV_LOG_ERROR, "VMD video: offset > width (%d > %d)\n",
                           ofs, frame_width);
                    break;
                }
                dp += s->frame.linesize[0];
                pp += s->prev_frame.linesize[0];
            }
            break;

        case 2:
            for (i = 0; i < frame_height; i++)
            {
                memcpy(dp, pb, frame_width);
                pb += frame_width;
                dp += s->frame.linesize[0];
                pp += s->prev_frame.linesize[0];
            }
            break;

        case 3:
            for (i = 0; i < frame_height; i++)
            {
                ofs = 0;
                do
                {
                    len = *pb++;
                    if (len & 0x80)
                    {
                        len = (len & 0x7F) + 1;
                        if (*pb++ == 0xFF)
                            len = rle_unpack(pb, &dp[ofs], len, frame_width - ofs);
                        else
                            memcpy(&dp[ofs], pb, len);
                        pb += len;
                        ofs += len;
                    }
                    else
                    {
                        /* interframe pixel copy */
                        if (ofs + len + 1 > frame_width)
                            return;
                        memcpy(&dp[ofs], &pp[ofs], len + 1);
                        ofs += len + 1;
                    }
                }
                while (ofs < frame_width);
                if (ofs > frame_width)
                {
                    av_log(s->avctx, AV_LOG_ERROR, "VMD video: offset > width (%d > %d)\n",
                           ofs, frame_width);
                }
                dp += s->frame.linesize[0];
                pp += s->prev_frame.linesize[0];
            }
            break;
        }
    }
}

static av_cold int vmdvideo_decode_init(AVCodecContext *avctx)
{
    VmdVideoContext *s = avctx->priv_data;
    int i;
    unsigned int *palette32;
    int palette_index = 0;
    unsigned char r, g, b;
    unsigned char *vmd_header;
    unsigned char *raw_palette;

    s->avctx = avctx;
    avctx->pix_fmt = PIX_FMT_PAL8;

    /* make sure the VMD header made it */
    if (s->avctx->extradata_size != VMD_HEADER_SIZE)
    {
        av_log(s->avctx, AV_LOG_ERROR, "VMD video: expected extradata size of %d\n",
               VMD_HEADER_SIZE);
        return -1;
    }
    vmd_header = (unsigned char *)avctx->extradata;

    s->unpack_buffer_size = AV_RL32(&vmd_header[800]);
    s->unpack_buffer = av_malloc(s->unpack_buffer_size);
    if (!s->unpack_buffer)
        return -1;

    /* load up the initial palette */
    raw_palette = &vmd_header[28];
    palette32 = (unsigned int *)s->palette;
    for (i = 0; i < PALETTE_COUNT; i++)
    {
        r = raw_palette[palette_index++] * 4;
        g = raw_palette[palette_index++] * 4;
        b = raw_palette[palette_index++] * 4;
        palette32[i] = (r << 16) | (g << 8) | (b);
    }

    return 0;
}

static int vmdvideo_decode_frame(AVCodecContext *avctx,
                                 void *data, int *data_size,
                                 AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    VmdVideoContext *s = avctx->priv_data;

    s->buf = buf;
    s->size = buf_size;

    if (buf_size < 16)
        return buf_size;

    s->frame.reference = 1;
    if (avctx->get_buffer(avctx, &s->frame))
    {
        av_log(s->avctx, AV_LOG_ERROR, "VMD Video: get_buffer() failed\n");
        return -1;
    }

    vmd_decode(s);

    /* make the palette available on the way out */
    memcpy(s->frame.data[1], s->palette, PALETTE_COUNT * 4);

    /* shuffle frames */
    FFSWAP(AVFrame, s->frame, s->prev_frame);
    if (s->frame.data[0])
        avctx->release_buffer(avctx, &s->frame);

    *data_size = sizeof(AVFrame);
    *(AVFrame *)data = s->prev_frame;

    /* report that the buffer was completely consumed */
    return buf_size;
}

static av_cold int vmdvideo_decode_end(AVCodecContext *avctx)
{
    VmdVideoContext *s = avctx->priv_data;

    if (s->prev_frame.data[0])
        avctx->release_buffer(avctx, &s->prev_frame);
    av_free(s->unpack_buffer);

    return 0;
}


/*
 * Audio Decoder
 */

#define BLOCK_TYPE_AUDIO    1
#define BLOCK_TYPE_INITIAL  2
#define BLOCK_TYPE_SILENCE  3

typedef struct VmdAudioContext
{
    AVCodecContext *avctx;
    int out_bps;
    int predictors[2];
} VmdAudioContext;

static const uint16_t vmdaudio_table[128] =
{
    0x000, 0x008, 0x010, 0x020, 0x030, 0x040, 0x050, 0x060, 0x070, 0x080,
    0x090, 0x0A0, 0x0B0, 0x0C0, 0x0D0, 0x0E0, 0x0F0, 0x100, 0x110, 0x120,
    0x130, 0x140, 0x150, 0x160, 0x170, 0x180, 0x190, 0x1A0, 0x1B0, 0x1C0,
    0x1D0, 0x1E0, 0x1F0, 0x200, 0x208, 0x210, 0x218, 0x220, 0x228, 0x230,
    0x238, 0x240, 0x248, 0x250, 0x258, 0x260, 0x268, 0x270, 0x278, 0x280,
    0x288, 0x290, 0x298, 0x2A0, 0x2A8, 0x2B0, 0x2B8, 0x2C0, 0x2C8, 0x2D0,
    0x2D8, 0x2E0, 0x2E8, 0x2F0, 0x2F8, 0x300, 0x308, 0x310, 0x318, 0x320,
    0x328, 0x330, 0x338, 0x340, 0x348, 0x350, 0x358, 0x360, 0x368, 0x370,
    0x378, 0x380, 0x388, 0x390, 0x398, 0x3A0, 0x3A8, 0x3B0, 0x3B8, 0x3C0,
    0x3C8, 0x3D0, 0x3D8, 0x3E0, 0x3E8, 0x3F0, 0x3F8, 0x400, 0x440, 0x480,
    0x4C0, 0x500, 0x540, 0x580, 0x5C0, 0x600, 0x640, 0x680, 0x6C0, 0x700,
    0x740, 0x780, 0x7C0, 0x800, 0x900, 0xA00, 0xB00, 0xC00, 0xD00, 0xE00,
    0xF00, 0x1000, 0x1400, 0x1800, 0x1C00, 0x2000, 0x3000, 0x4000
};

static av_cold int vmdaudio_decode_init(AVCodecContext *avctx)
{
    VmdAudioContext *s = avctx->priv_data;

    s->avctx = avctx;
    if (avctx->bits_per_coded_sample == 16)
        avctx->sample_fmt = AV_SAMPLE_FMT_S16;
    else
        avctx->sample_fmt = AV_SAMPLE_FMT_U8;
    s->out_bps = av_get_bits_per_sample_fmt(avctx->sample_fmt) >> 3;

    av_log(avctx, AV_LOG_DEBUG, "%d channels, %d bits/sample, "
           "block align = %d, sample rate = %d\n",
           avctx->channels, avctx->bits_per_coded_sample, avctx->block_align,
           avctx->sample_rate);

    return 0;
}

static void vmdaudio_decode_audio(VmdAudioContext *s, unsigned char *data,
                                  const uint8_t *buf, int buf_size, int stereo)
{
    int i;
    int chan = 0;
    int16_t *out = (int16_t *)data;

    for(i = 0; i < buf_size; i++)
    {
        if(buf[i] & 0x80)
            s->predictors[chan] -= vmdaudio_table[buf[i] & 0x7F];
        else
            s->predictors[chan] += vmdaudio_table[buf[i]];
        s->predictors[chan] = av_clip_int16(s->predictors[chan]);
        out[i] = s->predictors[chan];
        chan ^= stereo;
    }
}

static int vmdaudio_loadsound(VmdAudioContext *s, unsigned char *data,
                              const uint8_t *buf, int silent_chunks, int data_size)
{
    int silent_size = s->avctx->block_align * silent_chunks * s->out_bps;

    if (silent_chunks)
    {
        memset(data, s->out_bps == 2 ? 0x00 : 0x80, silent_size);
        data += silent_size;
    }
    if (s->avctx->bits_per_coded_sample == 16)
        vmdaudio_decode_audio(s, data, buf, data_size, s->avctx->channels == 2);
    else
    {
        /* just copy the data */
        memcpy(data, buf, data_size);
    }

    return silent_size + data_size * s->out_bps;
}

static int vmdaudio_decode_frame(AVCodecContext *avctx,
                                 void *data, int *data_size,
                                 AVPacket *avpkt)
{
    const uint8_t *buf = avpkt->data;
    int buf_size = avpkt->size;
    VmdAudioContext *s = avctx->priv_data;
    int block_type, silent_chunks;
    unsigned char *output_samples = (unsigned char *)data;

    if (buf_size < 16)
    {
        av_log(avctx, AV_LOG_WARNING, "skipping small junk packet\n");
        *data_size = 0;
        return buf_size;
    }

    block_type = buf[6];
    if (block_type < BLOCK_TYPE_AUDIO || block_type > BLOCK_TYPE_SILENCE)
    {
        av_log(avctx, AV_LOG_ERROR, "unknown block type: %d\n", block_type);
        return AVERROR(EINVAL);
    }
    buf      += 16;
    buf_size -= 16;

    silent_chunks = 0;
    if (block_type == BLOCK_TYPE_INITIAL)
    {
        uint32_t flags = AV_RB32(buf);
        silent_chunks  = av_popcount(flags);
        buf      += 4;
        buf_size -= 4;
    }
    else if (block_type == BLOCK_TYPE_SILENCE)
    {
        silent_chunks = 1;
        buf_size = 0; // should already be zero but set it just to be sure
    }

    /* ensure output buffer is large enough */
    if (*data_size < (avctx->block_align * silent_chunks + buf_size) * s->out_bps)
        return -1;

    *data_size = vmdaudio_loadsound(s, output_samples, buf, silent_chunks, buf_size);

    return avpkt->size;
}


/*
 * Public Data Structures
 */

AVCodec ff_vmdvideo_decoder =
{
    "vmdvideo",
    AVMEDIA_TYPE_VIDEO,
    CODEC_ID_VMDVIDEO,
    sizeof(VmdVideoContext),
    vmdvideo_decode_init,
    NULL,
    vmdvideo_decode_end,
    vmdvideo_decode_frame,
    CODEC_CAP_DR1,
    .long_name = NULL_IF_CONFIG_SMALL("Sierra VMD video"),
};

AVCodec ff_vmdaudio_decoder =
{
    "vmdaudio",
    AVMEDIA_TYPE_AUDIO,
    CODEC_ID_VMDAUDIO,
    sizeof(VmdAudioContext),
    vmdaudio_decode_init,
    NULL,
    NULL,
    vmdaudio_decode_frame,
    .long_name = NULL_IF_CONFIG_SMALL("Sierra VMD audio"),
};
