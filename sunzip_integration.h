typedef char sunzip_file_in; // unused
typedef int sunzip_file_out; // fd
#define sunzip_out_valid(x) ((x) != -1)
#define sunzip_out_invalid (-1)
//int sunzip_write(sunzip_file_out file, const void *buf, size_t sz);
#define sunzip_write write

sunzip_file_out sunzip_openout(const char *filename);
//int sunzip_closeout(sunzip_file_out file);
#define sunzip_closeout close
int sunzip_read(sunzip_file_in file, void *buffer, size_t size);
extern struct printbuffer_s sunzip_printb;
void PB_PrintString( struct printbuffer_s *pb, const char *fmt, ... );
#define sunzip_printout(...) PB_PrintString(&sunzip_printb, __VA_ARGS__)
#define sunzip_printerr(...) PB_PrintString(&sunzip_printb, __VA_ARGS__)
void sunzip_fatal( void );
void sunzip(sunzip_file_in file, int write);
