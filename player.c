/*
 *  player.c - example player for ffmpeg
 *
 *  Copyright (C) 2015 Intel Corporation
 *    Author: Zhao, Halley<halley.zhao@intel.com>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public License
 *  as published by the Free Software Foundation; either version 2.1
 *  of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free
 *  Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301 USA
 */

// gcc player.c `pkg-config --cflags --libs libavformat libavcodec libavutil` -o player

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55, 28, 1)
    #define av_frame_alloc avcodec_alloc_frame
    #if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(54, 28, 0)
        #define av_frame_free avcodec_free_frame
    #else
        #define av_frame_free av_freep
    #endif
#endif

#define PRINTF printf
#define DEBUG(format, ...)   printf("  %s, %d, " format, __FILE__, __LINE__, ##__VA_ARGS__)
#define ERROR(format, ...) fprintf(stderr, "!!ERROR  %s, %d, " format, __FILE__, __LINE__, ##__VA_ARGS__)

#ifndef ASSERT
#define ASSERT(expr) do {                                                                                               \
        if (!(expr))                                                                                                    \
            ERROR();                                                                                                    \
        assert(expr);                                                                                                   \
    } while(0)
#endif

static char* input_file = NULL;

int main(int argc, char *argv[])
{
    AVCodecContext* video_dec_ctx = NULL;
    AVCodec* video_dec = NULL;
    AVPacket pkt;
    AVFrame *frame = NULL;
    int read_eos = 0;
    int decode_count = 0;
    int render_count = 0;
    int video_stream_index = -1, i;
    FILE *dump_yuv = NULL;

    if (argc<2) {
        ERROR("no input file\n");
        return -1;
    }
    input_file = argv[1];

    // libav* init
    av_register_all();

    // open input file
    AVFormatContext* pFormat = NULL;
    if (avformat_open_input(&pFormat, input_file, NULL, NULL) < 0) {
        ERROR("fail to open input file: %s by avformat\n", input_file);
        return -1;
    }
    if (avformat_find_stream_info(pFormat, NULL) < 0) {
        ERROR("fail to find out stream info\n");
        return -1;
    }
    av_dump_format(pFormat,0,input_file,0);

    // find out video stream
    for (i = 0; i < pFormat->nb_streams; i++) {
        if (pFormat->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_dec_ctx = pFormat->streams[i]->codec;
            video_stream_index = i;
            break;
        }
    }
    ASSERT(video_dec_ctx && video_stream_index>=0);

    // open video codec
    video_dec = avcodec_find_decoder(video_dec_ctx->codec_id);
    if (avcodec_open2(video_dec_ctx, video_dec, NULL) < 0) {
        ERROR("fail to open codec\n");
        return -1;
    }

    // decode frames one by one
    av_init_packet(&pkt);
    while (1) {
        if(read_eos == 0 && av_read_frame(pFormat, &pkt) < 0) {
            read_eos = 1;
        }
        if (read_eos) {
            pkt.data = NULL;
            pkt.size = 0;
        }

        if (pkt.stream_index == video_stream_index) {
            frame = av_frame_alloc();
            int got_picture = 0,ret = 0;
            ret = avcodec_decode_video2(video_dec_ctx, frame, &got_picture, &pkt);
            if (ret < 0) { // decode fail (or decode finished)
                DEBUG("exit ...\n");
                break;
            }

            if (read_eos && ret>=0 && !got_picture) {
                DEBUG("ret=%d, exit ...\n", ret);
                break; // eos has been processed
            }

            decode_count++;
            if (got_picture) {
                // assumed I420 format
                int height[3] = {video_dec_ctx->height, video_dec_ctx->height/2, video_dec_ctx->height/2};
                int width[3] = {video_dec_ctx->width, video_dec_ctx->width/2, video_dec_ctx->width/2};
                int plane, row;

                if (!dump_yuv) {
                    char out_file[256];
                    sprintf(out_file, "./dump_%dx%d.I420", video_dec_ctx->width, video_dec_ctx->height);
                    dump_yuv = fopen(out_file, "ab");
                    if (!dump_yuv) {
                        ERROR("fail to create file for dumped yuv data\n");
                        return -1;
                    }
                    for (plane=0; plane<3; plane++) {
                        for (row = 0; row<height[plane]; row++)
                            fwrite(frame->data[plane]+ row*frame->linesize[plane], width[plane], 1, dump_yuv);
                    }
                }
                render_count++;
                av_frame_free(&frame);
            }
        }
    }
    if (dump_yuv)
        fclose(dump_yuv);
    PRINTF("decode %s ok, decode_count=%d, render_count=%d\n", input_file, decode_count, render_count);
    return 0;
}


