import sys
from PIL import Image

def reduce(c) :
    'R8G8B8 to R5G6B5'
    return ( (c[0]>>3)<<11 | (c[1]>>2)<<5 | c[2]>>3) 

sys.argv.append('terrain.png')
name = sys.argv[1]
img = Image.open(name).convert('RGB')
w,h=img.size
imgdata = tuple(reduce(c) for c in img.getdata())

palette = sorted(set(imgdata))

if len(palette)>256 : 
	print "too large palette, must be <256 colors"
	sys.exit(1) # error
print "#define mapWidth",w
print "#define mapHeight",h
print "const uint16_t palette[] = {"+','.join("0x%x"%c for c in palette )+"};"
print "const uint8_t worldMap[mapWidth][mapHeight]= {"
for line in range(h) :
	print '  {'+','.join("%2d"%palette.index(c) for c in imgdata[line*w:line*w+w])+'},'
print '};\n'
