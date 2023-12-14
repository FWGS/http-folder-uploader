static int memcmp(const void* buf1,
           const void* buf2,
           unsigned long count)
{
    if(!count)
        return(0);

    while(--count && *(char*)buf1 == *(char*)buf2 ) {
        buf1 = (char*)buf1 + 1;
        buf2 = (char*)buf2 + 1;
    }

    return(*((unsigned char*)buf1) - *((unsigned char*)buf2));
}

//#define memcmp __builtin_memcmp
#define va_list __builtin_va_list
//#define __NR_accept 43
#define INLINEMAIN
#include "bqc.h"

#ifdef FUCK_GOOGLE_YOU_ARE_COMPLETE_IDIOTS_YOU_MUST_EAT_SHIT_EVERY_GOOGLE_EMPLOYER_MUST_DIE_HARD_AND_BURN_IN_HELL_ETERNALLY
#undef accept
#ifdef __arm__ // fallback to keep compatibility with early kernels
static int accept(int sockfd, struct sockaddr *addr, void *addrlen)
{
	int ret = syscall( __NR_accept4, sockfd, addr, addrlen, 0 );
	if( ret == -ENOSYS )
		ret = syscall( __NR_accept, sockfd, addr, addrlen );
//	if( ret == -ENOSYS )
//		ret = 0;
	return ret;
}
#else
#define accept(x,y,z) accept4(x,y,z,0)
#endif
#endif
#undef tolower
#define tolower(x) (((x) > 96) && ((x) < 123)?((x) ^ 0x20):(x))
static __attribute__((hot)) int strncasecmp(const char *s1, const char *s2, long unsigned int n)
{
  if (n == 0)
    return 0;
//  write(1, s1, strlen(s1));
///  write(1, "\n", 1);
///  write(1, s2, strlen(s2));
///  write(1, "\n", 1);  
  while (n-- != 0 && tolower(*s1) == tolower(*s2))
    {
      if (n == 0 || *s1 == '\0' || *s2 == '\0')
    break;
      s1++;
      s2++;
    }

  return tolower(*(unsigned char *) s1) - tolower(*(unsigned char *) s2);
}
#define INT_MAX 0x7FFFFFFFL
#define UINT_MAX 0xFFFFFFFFL
#define ULONG_MAX 0xFFFFFFFFL

#define PATH_MAX 4096
#define SEEK_SET 0
typedef struct __dirstream DIR;
#define INADDR_ANY ((unsigned long int) 0x00000000)
#define SOMAXCONN 128
typedef uint32_t socklen_t;
typedef void* FILE;
#define _exit exit
//#define strcmp __builtin_strcmp
#define va_end __builtin_va_end
#define va_start __builtin_va_start
#define recv(a,b,c,d) read(a,b,c)
#define S_ISDIR(m) (((m) & S_IFMT) == S_IFDIR)
#define S_IFDIR 0040000
#define S_IFMT 00170000
#define S_IFREG  0100000

#ifdef newfstatat
#define stat stat64
#define stat(x,y) newfstatat(-100,x,y,0)
#else
#if defined __arm__
#undef stat
#define stat stat64
#endif
#endif
#ifndef open
#define open(...) openat(-100, __VA_ARGS__)
#endif
#ifndef unlink
#define unlink(x) unlinkat(-100,x)
#endif
#ifndef time
#define time(x) 1702051671
#endif
#ifndef mkdir
#define mkdir(x,y) mkdirat(-100,x,y);
#endif
#ifndef rename
#define rename(x,y) renameat(-100,x,-100,y)
#endif
#ifndef fork
#define fork()  clone( NULL, NULL, 17, NULL, NULL, NULL, NULL )
#endif
#ifndef NO_LOG
#define PrintWrap(...) PB_PrintString( &global_printbuf, __VA_ARGS__);write(1,global_printbuf_buffer, global_printbuf.pos);global_printbuf.pos = 0;
#define puts(x) write(1, (const char*)x, strlen(x))
#else
#define PrintWrap(...) (0)
#define puts(x) (0)
#endif
#define printf(...) PrintWrap(__VA_ARGS__)
#define fprintf(x,...) PrintWrap(__VA_ARGS__)

#define perror(x) puts(x)

#define fputs(x,y) puts(x)
#define fflush(x)
#define vfprintf(...)
#define putc(...)
#define sscanf(...) (0)

static int atoi(const char *s) {
    int acum = 0;
    int factor = 1;
    
    if(*s == '-') {
        factor = -1;
        s++;
    }
    
    while((*s >= '0')&&(*s <= '9')) {
      acum = acum * 10;
      acum = acum + (*s - 48);
      s++;
    }
    return (factor * acum);
}

static __attribute__((hot)) char* strcasestr(const char* haystack, const char* needle)
{
    char nch;
    char hch;

    if ((nch = *needle++) != 0) {
        size_t len = strlen(needle);
        do {
            do {
                if ((hch = *haystack++) == 0)
                    return NULL;
            } while (tolower(hch) != tolower(nch));
        } while (strncasecmp(haystack, needle, len) != 0);
        --haystack;
    }
    return (char*)(haystack);
}
static __attribute__((hot)) int strcmp(const char* s1, const char* s2)
{
    while(*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

struct linux_dirent {
#ifdef getdents
  uint32_t	d_ino;
  off_t		d_off;
  uint16_t	d_reclen;
#else
  uint64_t        d_ino;
  int64_t        d_off;
  unsigned short d_reclen;
  unsigned char  d_type;
#define getdents getdents64
#endif
  char		d_name[];
};
typedef long intptr_t;
typedef unsigned long uintptr_t;

#if defined __arm__ || defined __aarch64__ // W T F???
#define DIRECTORY_FLAG 040000
#else
#define DIRECTORY_FLAG 00200000
#endif
#define MAX_DIR_STACK 10
static struct dir_s {
	int fd;
	char buf[1024];
	int pos, count;
} dirstack[MAX_DIR_STACK];
static DIR *opendir(const char *name)
{
	int fd = open(name, O_RDONLY | DIRECTORY_FLAG);
	int i;
	if(fd < 0)
		return 0;
	for(i = 0; i < MAX_DIR_STACK; i++)
		if(dirstack[i].fd == 0) // note: make 0 invalid dir fd to skip dirstack initialization and leave it in .bss, but this is incorrect
		{
			dirstack[i].fd = fd;
			dirstack[i].pos = dirstack[i].count = 0;
			return (DIR*)&dirstack[i];
		}
	close(fd);
	return NULL;
}
//#define __NR_readdir 89
#define dirent linux_dirent
static struct linux_dirent* readdir(DIR* p)
{
	struct linux_dirent *ret = NULL;
	struct dir_s *dir = (struct dir_s*)p;
	if(!p)
		return NULL;

	if( dir->pos >= dir->count )
	{
		dir->count = getdents( dir->fd, dir->buf, sizeof( dir->buf ));
		dir->pos = 0;
		if( dir->count <= 0 )
			return NULL;
	}

	ret = (struct linux_dirent*)&dir->buf[dir->pos];
	dir->pos += ret->d_reclen;
	return ret;
}

static void closedir(DIR *p)
{
	struct dir_s *dir = (struct dir_s*)p;
	if(!p)
		return;
	close(dir->fd);
	dir->fd = 0;
}

#define va_arg __builtin_va_arg
#define bswap32(x) (((x) >> 24) | (((x) & 0x00FF0000) >> 8) | (((x) & 0x0000FF00) << 8) | ((x) << 24))
//#define bswap16(x) (((x) >> 8) | ((x) << 8))
#define ntohs(x) bswap16((unsigned short)x)
#undef ntohs
#define ntohs(x) bswap16((unsigned short)x)
#define STB_SPRINTF_IMPLEMENTATION
#define STB_SPRINTF_STATIC
#define STB_SPRINTF_NOFLOAT
#ifdef __arm__
#define STB_SPRINTF_NOUNALIGNED
#define NO_SR64
#define NO_DIV64
static int divide(int dividend, int divisor, int *rem);

#define div_const(x,y,z) divide(x,y,&z)
#define imod_wrap imod_wrap1
static int imod_wrap1(int x, int y)
{
	int m;
	divide(x,y, &m);
	return m;
}
#endif
#include "stb_sprintf.h"
#define snprintf stbsp_snprintf
#define vsnprintf stbsp_vsnprintf
static const char *inet_ntoa( const struct in_addr x )
{
	static char inet_printbuf[32];
	snprintf(inet_printbuf, 31, "0x%x", x.s_addr);
	return inet_printbuf;
}
#ifdef __arm__
#include "armeabi.h"
#endif
#define ALLOC_ALIGN 8U

static void *last_brk, *last_ptr;
static void *zcalloc(void *opaque, unsigned items, unsigned size)
{
	void *oldbrk;
	unsigned int sz = (items * size + (ALLOC_ALIGN - 1)) & ~(ALLOC_ALIGN - 1);
	if(!last_brk)
		last_brk = (void*)brk( NULL );
	oldbrk = last_brk;
	oldbrk = (void*)((((uintptr_t)oldbrk) + (ALLOC_ALIGN - 1)) & ~(ALLOC_ALIGN - 1 ));
	last_brk = (void*)brk( (char*)last_brk + sz );
	if( oldbrk == last_brk )
		return NULL;
	return (last_ptr = oldbrk);
}
static void *realloc(void *ptr, size_t size)
{
	size_t cursize = (char*)last_brk - (char*)ptr;
	void *newptr;
	if( size < cursize )
		cursize = size;
	if(ptr == last_ptr) // extend current break
	{
		if(cursize > size)
			last_brk = (void*)brk( (char*)last_brk + size - cursize );
		return ptr;
	}
	newptr = zcalloc( NULL, 1, size );
	if( newptr )
		memcpy( newptr, ptr, cursize );
	return newptr;
}
static void zcfree(void *opaque, void *ptr)
{
	// only allow "canceling" last allocation
	if(ptr == last_ptr)
		last_brk = last_ptr, last_ptr = NULL;
	// no free here, only run zip operations in forked processes!
}
#define malloc(x) zcalloc(NULL,1, x)
#define free(x) (void)(x)
#define fclose(...)
#define fdopen(...) NULL
#define fopen(...) NULL
#define fwrite(...) -1
#define fread(...) -1
#define ferror(...) 0

// stub, do not use
struct tm{
	int tm_mon, tm_min, tm_year, tm_sec, tm_hour, tm_mday;
};
static struct tm tm_stub;
#define localtime(...) &tm_stub
#define assert(x) if(!(x))puts(#x),_exit(127);

char * const z_errmsg[10] = {
    (char *)"need dictionary",     /* Z_NEED_DICT       2  */
    (char *)"stream end",          /* Z_STREAM_END      1  */
    (char *)"",                    /* Z_OK              0  */
    (char *)"file error",          /* Z_ERRNO         (-1) */
    (char *)"stream error",        /* Z_STREAM_ERROR  (-2) */
    (char *)"data error",          /* Z_DATA_ERROR    (-3) */
    (char *)"insufficient memory", /* Z_MEM_ERROR     (-4) */
    (char *)"buffer error",        /* Z_BUF_ERROR     (-5) */
    (char *)"incompatible version",/* Z_VERSION_ERROR (-6) */
    (char *)""
};

