// tiny wrapper around strange arduino libs to suit gifdec

// replaces open, read, lseek with own functions & provide to gifdec.h/c link

#include <FS.h>
extern "C" {
  int _open(const char *name, int mode);
  int _read(int fd, void *data, unsigned int num);
  int _lseek (int fd, int ofs, int whence);
  int _close(int fd);
}
// just one global file, just because.
File dataFile;

int _open(const char *name, int mode) {
    dataFile = SPIFFS.open(name,"r");
    return 1;
}

int _read(int fd, void *data, unsigned int num)
{
    char *dst = (char *)data;
    for (int i=0;i<num;i++) {
        *dst++=dataFile.read();
    }
}

int _lseek (int fd, int ofs, int whence)
{
    unsigned int start = 0;
    if (whence == SEEK_CUR) {
        start = dataFile.position();
    }
    dataFile.seek(start+ofs,SeekSet);
    return start+ofs;
}

int _close(int fd)
{
    dataFile.close();
    return 0;    
}
