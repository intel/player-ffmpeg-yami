all:
	rm -f player
	gcc player.c video_gl_render.c gles2_help.c egl_util.c -g `pkg-config --cflags --libs libavformat libavcodec libavutil gl egl` -lX11 -o player
