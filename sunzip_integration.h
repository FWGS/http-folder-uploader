typedef char sunzip_file_in; // unused
typedef int sunzip_file_out; // fd
#define sunzip_out_valid(x) ((x) != -1)
#define sunzip_out_invalid (-1)
int sunzip_write(sunzip_file_out file, const void *buf, size_t sz);

sunzip_file_out sunzip_openout(const char *filename);
int sunzip_closeout(sunzip_file_out file);
int sunzip_read(sunzip_file_in file, void *buffer, size_t size);
