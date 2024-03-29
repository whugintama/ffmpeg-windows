/*
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
//****************************************************************************//
//libavutil\samplefmt.h,libavutil\samplefmt.c
//	有关采样率的相关信息
//学习的地方：
//1.采样率是如何计算和如何存储数据的？
//附录：
//1.扩展知识----采样率简介.txt
//****************************************************************************//


#ifndef AVUTIL_SAMPLEFMT_H
#define AVUTIL_SAMPLEFMT_H

#include "avutil.h"

/**
 * all in native-endian format
 */
FFMPEGLIB_API enum AVSampleFormat
{
    AV_SAMPLE_FMT_NONE = -1,
    AV_SAMPLE_FMT_U8,          ///< unsigned 8 bits
    AV_SAMPLE_FMT_S16,         ///< signed 16 bits
    AV_SAMPLE_FMT_S32,         ///< signed 32 bits
    AV_SAMPLE_FMT_FLT,         ///< float
    AV_SAMPLE_FMT_DBL,         ///< double
    AV_SAMPLE_FMT_NB           ///< Number of sample formats. DO NOT USE if linking dynamically
};

/**
 * Return the name of sample_fmt, or NULL if sample_fmt is not
 * recognized.
 */
FFMPEGLIB_API const char *av_get_sample_fmt_name(enum AVSampleFormat sample_fmt);

/**
 * Return a sample format corresponding to name, or AV_SAMPLE_FMT_NONE
 * on error.
 */
FFMPEGLIB_API enum AVSampleFormat av_get_sample_fmt(const char *name);

/**
 * Generate a string corresponding to the sample format with
 * sample_fmt, or a header if sample_fmt is negative.
 *
 * @param buf the buffer where to write the string
 * @param buf_size the size of buf
 * @param sample_fmt the number of the sample format to print the
 * corresponding info string, or a negative value to print the
 * corresponding header.
 * @return the pointer to the filled buffer or NULL if sample_fmt is
 * unknown or in case of other errors
 */
FFMPEGLIB_API char *av_get_sample_fmt_string(char *buf, int buf_size, enum AVSampleFormat sample_fmt);

/**
 * Return sample format bits per sample.
 *
 * @param sample_fmt the sample format
 * @return number of bits per sample or zero if unknown for the given
 * sample format
 */
FFMPEGLIB_API int av_get_bits_per_sample_fmt(enum AVSampleFormat sample_fmt);

/**
 * Fill channel data pointers and linesizes for samples with sample
 * format sample_fmt.
 *
 * The pointers array is filled with the pointers to the samples data:
 * data[c] points to the first sample of channel c.
 * data[c] + linesize[0] points to the second sample of channel c
 *
 * @param pointers array to be filled with the pointer for each plane, may be NULL
 * @param linesizes array to be filled with the linesize, may be NULL
 * @param buf the pointer to a buffer containing the samples
 * @param nb_samples the number of samples in a single channel
 * @param planar 1 if the samples layout is planar, 0 if it is packed
 * @param nb_channels the number of channels
 * @return the total size of the buffer, a negative
 * error code in case of failure
 */
FFMPEGLIB_API int av_samples_fill_arrays(uint8_t *pointers[8], int linesizes[8],
        uint8_t *buf, int nb_channels, int nb_samples,
        enum AVSampleFormat sample_fmt, int planar, int align);

/**
 * Allocate a samples buffer for nb_samples samples, and
 * fill pointers and linesizes accordingly.
 * The allocated samples buffer has to be freed by using
 * av_freep(&pointers[0]).
 *
 * @param nb_samples number of samples per channel
 * @param planar 1 if the samples layout is planar, 0 if packed,
 * @param align the value to use for buffer size alignment
 * @return the size in bytes required for the samples buffer, a negative
 * error code in case of failure
 * @see av_samples_fill_arrays()
 */
FFMPEGLIB_API int av_samples_alloc(uint8_t *pointers[8], int linesizes[8],
                                   int nb_samples, int nb_channels,
                                   enum AVSampleFormat sample_fmt, int planar,
                                   int align);

#endif /* AVCORE_SAMPLEFMT_H */
