all:
	gcc player.c `pkg-config --cflags --libs libavformat libavcodec libavutil` -o player
