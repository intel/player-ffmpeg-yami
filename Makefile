player:
	rm -f player
	gcc player.c video_gl_render.c gles2_help.c egl_util.c `pkg-config --cflags --libs libavformat libavcodec libavutil egl gl` -lX11 -o player

player_debug:
	rm -f player_debug
	gcc player.c video_gl_render.c gles2_help.c egl_util.c -g -DPLAYER_DEBUG `pkg-config --cflags --libs libavformat libavcodec libavutil egl gl` -lX11 -o player_debug

ffmpeg:clone-ffmpeg apply-patches build-ffmpeg

clone-ffmpeg:ext/ffmpeg/configure
ext/ffmpeg/configure:
	echo "clone ffmpeg ..."
	git submodule init && git submodule update

apply-patches:
	echo "apply ffmpeg patches for yami ..."
	cd ext/ffmpeg && git am ../../patches/*.patch
	git commit -a -m "ffmpeg submodule update after yami patches"

build-ffmpeg:
	echo "build ffmpeg ..."
	cd ext/ffmpeg && ./configure --prefix=${FFMPEG_PREFIX} --enable-libyami-h264 --disable-doc --disable-stripping --enable-shared --enable-debug=3 && make -j8 && make install
