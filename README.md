it is an example player basing on ffmpeg, with addtional patches (to ffmpeg) to use libyami for decoding/encoding.
for now only h264 decoder is enabled.

build steps:
1. make sure libva/libyami are installed in your environemntsi: https://github.com/01org/libyami/wiki
2. setup env for ffmpeg build: "source build-ffmpeg.env"
3. build ffmpeg (with yami patches): "make ffmpeg"
   by default, ffmpeg is installed to /opt/ffmpeg
4. build the example player: "make player"
