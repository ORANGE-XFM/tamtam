upload: 
	pio run -t upload

terrain.h: mk_terrain.py terrain.png
	python mk_terrain.py terrain.png > terrain.h

clean:
	rm -f terrain.h