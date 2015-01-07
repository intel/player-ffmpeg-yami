/*
 *  video_gl_render.c - simple texture rendering for video frame with gles v2
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

#include "gles2_help.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "EGL/eglext.h"
#include "egl_util.h"
#include "video_gl_render.h"

static int init_egl(uint32_t width, uint32_t height, int is_dmabuf);
static EGLContextType *egl_context = NULL;
static Display * x11_display = NULL;
static Window x11_window = 0;

#define EGL_CHECK_RESULT_RET(result, promptStr, ret) do {   \
    if (result != EGL_TRUE) {                               \
        ERROR("%s failed", promptStr);                      \
        return ret;                                          \
    }                                                       \
}while(0)
#define CHECK_HANDLE_RET(handle, invalid, promptStr, ret) do {  \
    if (handle == invalid) {                                    \
        ERROR("%s failed", promptStr);                          \
        return ret;                                              \
    }                                                           \
} while(0)

static GLuint
createLumaTexture(GLubyte *pixels, GLuint width, GLuint height)
{
    GLuint textureId;

    glGenTextures(1, &textureId );
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width/4, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    return textureId;
}

static GLuint
createTextureFromEgl(EGLImageKHR egl_image, GLenum target, GLuint width, GLuint height, GLuint pitch)
{
    GLuint textureId;

    DEBUG("create texture from egl image\n");
    glGenTextures(1, &textureId );
    glBindTexture(target, textureId);
    if (egl_image != EGL_NO_IMAGE_KHR) {
        glEGLImageTargetTexture2DOES(target, egl_image);
    } else {
        ERROR("fail to create EGLImage from dma_buf");
    }


    return textureId;
}

// test use only
static GLuint
createTestTexture( )
{
    GLuint textureId;
    // 2x2 Image, 4 bytes per pixel (R, G, B, A)
    GLubyte pixels[4 * 4] =
    {
      255,   0,   0, 255,   // Red
        0, 255,   0, 255,   // Green
        0,   0, 255, 255,   // Blue
      255, 255, 255, 255,   // White
    };

    glGenTextures(1, &textureId );
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);

    return textureId;
}

int drawVideo(uintptr_t handle, int type, uint32_t width, uint32_t height, uint32_t pitch)
{
    GLuint tex = 0;
    EGLImageKHR egl_image = EGL_NO_IMAGE_KHR;
    GLenum target = GL_TEXTURE_2D;

    DEBUG("handle=%p, width=%d, height=%d, pitch=%d\n", (void*)handle, width, height, pitch);
    if (!egl_context)
        init_egl(width, height, type == 2);

    switch (type) {
    case 0:
        // HACK, simple draw luma as RGBX
        tex = createLumaTexture((uint8_t*)handle, width, height);
        break;
    case 1:
    case 2:
        if (type == 2)
            target = GL_TEXTURE_EXTERNAL_OES;

        egl_image = createEglImageFromHandle(egl_context->eglContext.display, egl_context->eglContext.context,
           target == GL_TEXTURE_EXTERNAL_OES, handle, width, height, pitch);

        if (egl_image != EGL_NO_IMAGE_KHR) {
            tex = createTextureFromEgl(egl_image, target, width, height, pitch);
        } else {
            ERROR("fail to create EGLImage from dma_buf");
            return -1;
        }
        break;
    default:
        ERROR("unknonw video buffer type\n");
        return -1;
    }
    // GLuint tex = createTestTexture();
    glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    drawTextures(egl_context, target, &tex, 1);
    glDeleteTextures(1, &tex);
    if (egl_image != EGL_NO_IMAGE_KHR) {
        eglDestroyImageKHR(egl_context->eglContext.display, egl_image);
    }

    return 0;
}


static int init_egl(uint32_t width, uint32_t height, int is_dmabuf)
{
    DEBUG("setup X connection and egl environments\n");

    XInitThreads();
    x11_display = XOpenDisplay(NULL);
    CHECK_HANDLE_RET(x11_display, NULL, "XOpenDisplay", -1);
    Window x11_root_window = DefaultRootWindow(x11_display);

    // create with video size, simplify it
    x11_window = XCreateSimpleWindow(x11_display, x11_root_window,
        0, 0, width, height, 0, 0, WhitePixel(x11_display, 0));
    XMapWindow(x11_display, x11_window);
    XSync(x11_display, 0);

    egl_context = eglInit(x11_display, x11_window, 0, is_dmabuf);
}
int deinit_egl()
{
    DEBUG("deinit_egl ...\n");
    if (!egl_context)
        return 0;

    eglRelease(egl_context);
    if (x11_window && x11_display) {
        XUnmapWindow(x11_display, x11_window);
        XDestroyWindow(x11_display, x11_window);
    }
    if (!x11_display)
        XCloseDisplay(x11_display);

    egl_context = NULL;
    DEBUG("deinit_egl successfully\n");
    return 0;
}

