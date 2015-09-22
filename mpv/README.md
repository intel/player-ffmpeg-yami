please follow below steps to enable the libyami decoding for mpv player:
1. make sure libva/libyami are installed in your environemnts: https://github.com/01org/libyami/wiki
2. refer to https://github.com/mpv-player/mpv-build to fetch the mpv code.
3. apply the following two patchs:
   0001-add-enable-libyami-to-ffmpeg-build-options.patch
   0002-avcodec-h264-enable-libyami-h264-decoder.patch
4. rebuild the mpv code with libyami enabled
5. run mpv player with command "mpv --hwdec=vaapi xxxxx.264"
