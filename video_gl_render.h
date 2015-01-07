/*
 *  video_render_gl.h - simple texture rendering for video frame with gles v2
 *
 *  Copyright (C) 2013-2014 Intel Corporation
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

#ifndef __VIDEO_GL_RENDER_H__
#define __VIDEO_GL_RENDER_H__

#include <stdint.h>
#include <stdio.h>
#include <assert.h>

// type 0: raw yuv data, 1: drm name (flink), 2: dma_buf handle
int drawVideo(uintptr_t handle, int type, uint32_t width, uint32_t height, uint32_t pitch);
// int init_egl(uint32_t width, uint32_t height, int is_dmabuf);
int deinit_egl();

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

#endif // __VIDEO_GL_RENDER_H__
