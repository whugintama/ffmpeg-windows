/*
 * Copyright (c) 2008 Vitor Sessak
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
 * memory buffer source filter
 */

#include "avfilter.h"
#include "vsrc_buffer.h"
#include "libavutil/imgutils.h"

typedef struct {
    int64_t           pts;
    AVFrame           frame;
    int               has_frame;
    int               h, w;
    enum PixelFormat  pix_fmt;
    AVRational        time_base;     ///< time_base to set in the output link
    AVRational        pixel_aspect;
} BufferSourceContext;

int av_vsrc_buffer_add_frame2(AVFilterContext *buffer_filter, AVFrame *frame,
                              int64_t pts, AVRational pixel_aspect, int width,
                              int height, enum PixelFormat  pix_fmt,
                              const char *sws_param)
{
    BufferSourceContext *c = buffer_filter->priv;
    int ret;

    if (c->has_frame) {
        av_log(buffer_filter, AV_LOG_ERROR,
               "Buffering several frames is not supported. "
               "Please consume all available frames before adding a new one.\n"
            );
        //return -1;
    }

    if(width != c->w || height != c->h || pix_fmt != c->pix_fmt){
        AVFilterContext *scale= buffer_filter->outputs[0]->dst;
        AVFilterLink *link;

        av_log(buffer_filter, AV_LOG_INFO, "Changing filter graph input to accept %dx%d %d (%d %d)\n",
               width,height,pix_fmt, c->pix_fmt, scale->outputs[0]->format);

        if(!scale || strcmp(scale->filter->name,"scale")){
            AVFilter *f= avfilter_get_by_name("scale");

            av_log(buffer_filter, AV_LOG_INFO, "Inserting scaler filter\n");
            if(avfilter_open(&scale, f, "Input equalizer") < 0)
                return -1;

            if((ret=avfilter_init_filter(scale, sws_param, NULL))<0){
                avfilter_free(scale);
                return ret;
            }

            if((ret=avfilter_insert_filter(buffer_filter->outputs[0], scale, 0, 0))<0){
                avfilter_free(scale);
                return ret;
            }

            scale->outputs[0]->format= c->pix_fmt;
        }

        c->pix_fmt= scale->inputs[0]->format= pix_fmt;
        c->w= scale->inputs[0]->w= width;
        c->h= scale->inputs[0]->h= height;

        link= scale->outputs[0];
        if ((ret =  link->srcpad->config_props(link)) < 0)
            return ret;
    }

    memcpy(c->frame.data    , frame->data    , sizeof(frame->data));
    memcpy(c->frame.linesize, frame->linesize, sizeof(frame->linesize));
    c->frame.interlaced_frame= frame->interlaced_frame;
    c->frame.top_field_first = frame->top_field_first;
    c->frame.key_frame = frame->key_frame;
    c->frame.pict_type = frame->pict_type;
    c->pts = pts;
    c->pixel_aspect = pixel_aspect;
    c->has_frame = 1;

    return 0;
}

int av_vsrc_buffer_add_frame(AVFilterContext *buffer_filter, AVFrame *frame,
                             int64_t pts, AVRational pixel_aspect)
{
    BufferSourceContext *c = buffer_filter->priv;

    return av_vsrc_buffer_add_frame2(buffer_filter, frame,
                              pts, pixel_aspect, c->w,
                              c->h, c->pix_fmt, "");
}

static av_cold int init(AVFilterContext *ctx, const char *args, void *opaque)
{
    BufferSourceContext *c = ctx->priv;
    char pix_fmt_str[128];
    int n = 0;

    if (!args ||
        (n = sscanf(args, "%d:%d:%127[^:]:%d:%d:%d:%d", &c->w, &c->h, pix_fmt_str,
                    &c->time_base.num, &c->time_base.den,
                    &c->pixel_aspect.num, &c->pixel_aspect.den)) != 7) {
        av_log(ctx, AV_LOG_ERROR, "Expected 7 arguments, but %d found in '%s'\n", n, args);
        return AVERROR(EINVAL);
    }
    if ((c->pix_fmt = av_get_pix_fmt(pix_fmt_str)) == PIX_FMT_NONE) {
        char *tail;
        c->pix_fmt = strtol(pix_fmt_str, &tail, 10);
        if (*tail || c->pix_fmt < 0 || c->pix_fmt >= PIX_FMT_NB) {
            av_log(ctx, AV_LOG_ERROR, "Invalid pixel format string '%s'\n", pix_fmt_str);
            return AVERROR(EINVAL);
        }
    }

    av_log(ctx, AV_LOG_INFO, "w:%d h:%d pixfmt:%s\n", c->w, c->h, (av_getav_pix_fmt_descriptors())[c->pix_fmt].name);
    return 0;
}

static int query_formats(AVFilterContext *ctx)
{
    BufferSourceContext *c = ctx->priv;
    enum PixelFormat pix_fmts[] = { c->pix_fmt, PIX_FMT_NONE };

    avfilter_set_common_formats(ctx, avfilter_make_format_list(pix_fmts));
    return 0;
}

static int config_props(AVFilterLink *link)
{
    BufferSourceContext *c = link->src->priv;

    link->w = c->w;
    link->h = c->h;
    link->sample_aspect_ratio = c->pixel_aspect;
    link->time_base = c->time_base;

    return 0;
}

static int request_frame(AVFilterLink *link)
{
    BufferSourceContext *c = link->src->priv;
    AVFilterBufferRef *picref;

    if (!c->has_frame) {
        av_log(link->src, AV_LOG_ERROR,
               "request_frame() called with no available frame!\n");
        //return -1;
    }

    /* This picture will be needed unmodified later for decoding the next
     * frame */
    picref = avfilter_get_video_buffer(link, AV_PERM_WRITE | AV_PERM_PRESERVE |
                                       AV_PERM_REUSE2,
                                       link->w, link->h);

    av_image_copy(picref->data, picref->linesize,
                  c->frame.data, c->frame.linesize,
                  picref->format, link->w, link->h);

    picref->pts                    = c->pts;
    picref->video->pixel_aspect    = c->pixel_aspect;
    picref->video->interlaced      = c->frame.interlaced_frame;
    picref->video->top_field_first = c->frame.top_field_first;
    picref->video->key_frame       = c->frame.key_frame;
    picref->video->pict_type       = c->frame.pict_type;
    avfilter_start_frame(link, avfilter_ref_buffer(picref, ~0));
    avfilter_draw_slice(link, 0, link->h, 1);
    avfilter_end_frame(link);
    avfilter_unref_buffer(picref);

    c->has_frame = 0;

    return 0;
}

static int poll_frame(AVFilterLink *link)
{
    BufferSourceContext *c = link->src->priv;
    return !!(c->has_frame);
}

AVFilter avfilter_vsrc_buffer = {
    .name      = "buffer",
    .description = NULL_IF_CONFIG_SMALL("Buffer video frames, and make them accessible to the filterchain."),
    .priv_size = sizeof(BufferSourceContext),
    .query_formats = query_formats,

    .init      = init,

    .inputs    = (AVFilterPad[]) {{ .name = NULL }},
    .outputs   = (AVFilterPad[]) {{ .name            = "default",
                                    .type            = AVMEDIA_TYPE_VIDEO,
                                    .request_frame   = request_frame,
                                    .poll_frame      = poll_frame,
                                    .config_props    = config_props, },
                                  { .name = NULL}},
};
