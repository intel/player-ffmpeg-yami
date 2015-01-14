/*
 *  player.c - example player for ffmpeg-yami
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

#include <string.h>
#include <unistd.h>
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/frame.h>
#include "video_gl_render.h"
#if LIBAVCODEC_VERSION_INT < AV_VERSION_INT(55, 28, 1)
    #define av_frame_alloc avcodec_alloc_frame
    #if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(54, 28, 0)
        #define av_frame_free avcodec_free_frame
    #else
        #define av_frame_free av_freep
    #endif
#endif

static char* input_file = NULL;
static int render_mode = 0;

static void print_help(const char* app)
{
    PRINTF("%s <options>\n", app);
    PRINTF("   -i media file to decode\n");
    PRINTF("   -m <render mode>\n");
    PRINTF("      0: dump video frame to file\n");
    PRINTF("      1: upload raw video frame (Y) as texture\n");
    PRINTF("      2: texture: export video frame as drm name (RGBX) + texture from drm name\n");
    PRINTF("      3: texture: export video frame as dma_buf(RGBX) + texutre from dma_buf\n");
}

static int process_cmdline(int argc, char *argv[])
{
    char opt;

    while ((opt = getopt(argc, argv, "h:m:i:?")) != -1)
    {
        switch (opt) {
        case 'h':
        case '?':
            print_help (argv[0]);
            return -1;
        case 'i':
            input_file = optarg;
            break;
        case 'm':
            render_mode = atoi(optarg);
            break;
        default:
            print_help(argv[0]);
            break;
        }
    }
    PRINTF("input file: %s, render_mode: %d\n", input_file, render_mode);

    return 0;
}

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
    uint8_t *frame_copy = NULL;
    FILE *dump_yuv = NULL;

    // parse command line parameters
    process_cmdline(argc, argv);
    if (!input_file) {
        ERROR("no input file specified\n");
        return -1;
    }

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
    video_dec_ctx->coder_type = render_mode ? render_mode -1 : render_mode; // specify output frame type
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
                switch (render_mode) {
                case 0: // dump raw video frame to disk file
                case 1: { // draw raw frame data as texture
                    // assumed I420 format
                    int height[3] = {video_dec_ctx->height, video_dec_ctx->height/2, video_dec_ctx->height/2};
                    int width[3] = {video_dec_ctx->width, video_dec_ctx->width/2, video_dec_ctx->width/2};
                    int plane, row;

                    if (render_mode == 0) {
                        if (!dump_yuv) {
                            char out_file[256];
                            sprintf(out_file, "./dump_%dx%d.I420", video_dec_ctx->width, video_dec_ctx->height);
                            dump_yuv = fopen(out_file, "ab");
                            if (!dump_yuv) {
                                ERROR("fail to create file for dumped yuv data\n");
                                return -1;
                            }
                        }
                        for (plane=0; plane<3; plane++) {
                            for (row = 0; row<height[plane]; row++)
                                fwrite(frame->data[plane]+ row*frame->linesize[plane], width[plane], 1, dump_yuv);
                        }
                    } else {
                        // glTexImage2D  doesn't handle pitch, make a copy of video data
                        frame_copy = malloc(video_dec_ctx->height * video_dec_ctx->width * 3 / 2);
                        unsigned char* ptr = frame_copy;

                        for (plane=0; plane<3; plane++) {
                            for (row=0; row<height[plane]; row++) {
                                memcpy(ptr, frame->data[plane]+row*frame->linesize[plane], width[plane]);
                                ptr += width[plane];
                            }
                        }

                        drawVideo((uintptr_t)frame_copy, 0, video_dec_ctx->width, video_dec_ctx->height, 0);
                    }
                }
                    break;
                case 2: // draw video frame as texture with drm handle
                case 3: // draw video frame as texture with dma_buf handle
                    drawVideo((uintptr_t)frame->data[0], render_mode -1, video_dec_ctx->width, video_dec_ctx->height, (uintptr_t)frame->linesize[0]);
                    break;
                default:
                    break;
                }
                render_count++;
            }
        }
    }

    if (frame)
        av_frame_free(&frame);
    if (frame_copy)
        free(frame_copy);
    if (dump_yuv)
        fclose(dump_yuv);
    deinit_egl();
    PRINTF("decode %s ok, decode_count=%d, render_count=%d\n", input_file, decode_count, render_count);

    return 0;
}


