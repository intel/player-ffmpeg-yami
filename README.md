it is an example player basing on ffmpeg, with addtional patches (to ffmpeg) to use libyami for decoding/encoding.
for now only h264 decoder is enabled.

build steps:
1. make sure libva/libyami are installed in your environemntsi: https://github.com/01org/libyami/wiki
2. setup env for ffmpeg build: "source build-ffmpeg.env"
3. build ffmpeg (with yami patches): "make ffmpeg"
   by default, ffmpeg is installed to /opt/ffmpeg
4. build the example player: "make player"


###relicense
At 2016/6/8, we relicense libyami from LGPL to Apache V2. You need passing --enable-version3 to ffmpeg and dynamic link libyami to make license compatible.
You can refer to http://ffmpeg.org/general.html#OpenCORE_002c-VisualOn_002c-and-Fraunhofer-libraries for details.
You can use old LGPL licensed libyami at https://github.com/01org/libyami/tree/master. But we freezed it as 0.3.2.

###deprecated
We abandon this project since we have a better replacement. Please use https://github.com/01org/ffmpeg_libyami for ffmpeg integration.