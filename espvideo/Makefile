play: gifplayer data/video.gif
	./gifplayer data/video.gif
video.mp4: 
	youtube-dl https://www.youtube.com/watch?v=2WBG2rJZGW8 -f 160 -o $@
#consider also https://www.youtube.com/watch?v=dQw4w9WgXcQ

data/video.gif: video.mp4
	ffmpeg -i $^ -ss 00:04:15 -to 00:08:00 -vf "fps=15,scale=-1:16:flags=lanczos,split[s0][s1];[s0]palettegen[p];[s1][p]paletteuse" -loop 1 $@

gifplayer: gifdec/gifdec.c gifdec/gifdec.h gifdec/example.c 
	cc -o $@ gifdec/gifdec.c gifdec/example.c `pkg-config --cflags --libs sdl2` 

clean: 
	rm gifplayer video.gif video.mp4 video.c
