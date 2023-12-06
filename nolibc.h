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
#include "bqc.h"
#undef tolower
#define tolower(x) (((x) > 96) && ((x) < 123)?((x) ^ 0x20):(x))
static int strncasecmp(const char *s1, const char *s2, long unsigned int n)
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
#ifdef __arm__
#undef stat
#define stat stat64
#else
#define S_ISDIR(m) (m > 1) // WTF??? stat seems to be completely broken on x86_64?
#endif

#define PrintWrap(...) PB_PrintString( &global_printbuf, __VA_ARGS__);write(1,global_printbuf_buffer, global_printbuf.pos);global_printbuf.pos = 0;

#define printf(...) PrintWrap(__VA_ARGS__)
#define Error(...) PrintWrap(__VA_ARGS__)
#define Report(...) PrintWrap(__VA_ARGS__)

#define perror(x) puts(x)

#define puts(x) write(1, (const char*)x, strlen(x))
#define fputs(x,y) puts(x)
#define fflush(x)

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

static char* strcasestr(const char* haystack, const char* needle)
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
static int strcmp(const char* s1, const char* s2)
{
    while(*s1 && (*s1 == *s2))
    {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

struct linux_dirent {
  uint32_t	d_ino;
  off_t		d_off;
  uint16_t	d_reclen;
  char		d_name[];
};
typedef long intptr_t;

#ifdef __arm__ // W T F???
#define DIRECTORY_FLAG 040000
#else
#define DIRECTORY_FLAG 00200000
#endif
static DIR *opendir(const char *name)
{
	int fd = open(name, O_RDONLY | DIRECTORY_FLAG);
	if(fd >= 0)
		return (DIR*)(intptr_t)(fd+1);
	return 0;
}
//#define __NR_readdir 89
#define dirent linux_dirent
static struct linux_dirent* readdir(DIR* p)
{
	static char dirents_buffer[2048];
	struct linux_dirent *ret = NULL;
	static int pos, count;
	
	int fd = (int)(intptr_t)p - 1;

	if(!p)
		return NULL;
	if( pos >= count )
	{
		count = getdents(fd, dirents_buffer, sizeof( dirents_buffer ));
		pos = 0;
		if( count <= 0 )
			return NULL;
	}

	ret = (struct linux_dirent*)&dirents_buffer[pos];
	pos += ret->d_reclen;
	return ret;
}

static void closedir(DIR *p)
{
	int fd = (int)(intptr_t)p - 1;
	if(!p)
		return;
	close(fd);
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
#if 0
#define du_int unsigned long long int
#define su_int unsigned int
#define di_int long long int
typedef union 
{	unsigned long long all;
	struct{
		unsigned int low;
		int high;
	}s;
} udwords;

unsigned int __udivmodsi4(unsigned int divident, unsigned int divisor, unsigned int *remainder);

du_int __attribute((optimize(0)))
__udivmoddi4(du_int a, du_int b, du_int* rem)
{
    const unsigned n_uword_bits = sizeof(su_int) * CHAR_BIT;
    const unsigned n_udword_bits = sizeof(du_int) * CHAR_BIT;
    udwords n;
    n.all = a;
    udwords d;
    d.all = b;
    udwords q;
    udwords r;	
    unsigned sr;
    /* special cases, X is unknown, K != 0 */
    if (n.s.high == 0)
    {
        if (d.s.high == 0)
        {
            /* 0 X
             * ---
             * 0 X
             */
//            if (rem)
  //              *rem = n.s.low % d.s.low;
            return __udivmodsi4(n.s.low,d.s.low,rem);//((unsigned long long)n.s.low) / ((unsigned long long)d.s.low);
        }
        /* 0 X
         * ---
         * K X
         */
        if (rem)
            *rem = n.s.low;
        return 0;
    }
    /* n.s.high != 0 */
    if (d.s.low == 0)
    {
        if (d.s.high == 0)
        {
            /* K X
             * ---
             * 0 0
             */ 
            //if (rem)
//                *rem = n.s.high % d.s.low;
            //return (unsigned long)n.s.high / (unsigned long)d.s.low;
            return __udivmodsi4(n.s.high,d.s.low,rem);
            
        }
        /* d.s.high != 0 */
        if (n.s.low == 0)
        {
            /* K 0
             * ---
             * K 0
             */
            return  __udivmodsi4(n.s.high,d.s.high,rem);
            /*if (rem)
            {
                r.s.high = n.s.high % d.s.high;
                r.s.low = 0;
                *rem = r.all;
            }
            return n.s.high / d.s.high;*/
        }
        /* K K
         * ---
         * K 0
         */
        if ((d.s.high & (d.s.high - 1)) == 0)     /* if d is a power of 2 */
        {
            if (rem)
            {
                r.s.low = n.s.low;
                r.s.high = n.s.high & (d.s.high - 1);
                *rem = r.all;
            }
            return n.s.high >> __builtin_ctz(d.s.high);
        }
        /* K K
         * ---
         * K 0
         */
        sr = __builtin_clz(d.s.high) - __builtin_clz(n.s.high);
        /* 0 <= sr <= n_uword_bits - 2 or sr large */
        if (sr > n_uword_bits - 2)
        {
           if (rem)
                *rem = n.all;
            return 0;
        }
        ++sr;
        /* 1 <= sr <= n_uword_bits - 1 */
        /* q.all = n.all << (n_udword_bits - sr); */
        q.s.low = 0;
        q.s.high = n.s.low << (n_uword_bits - sr);
        /* r.all = n.all >> sr; */
        r.s.high = n.s.high >> sr;
        r.s.low = (n.s.high << (n_uword_bits - sr)) | (n.s.low >> sr);
    }
    else  /* d.s.low != 0 */
    {
        if (d.s.high == 0)
        {
            /* K X
             * ---
             * 0 K
             */
            if ((d.s.low & (d.s.low - 1)) == 0)     /* if d is a power of 2 */
            {
                if (rem)
                    *rem = n.s.low & (d.s.low - 1);
                if (d.s.low == 1)
                    return n.all;
                sr = __builtin_ctz(d.s.low);
                q.s.high = n.s.high >> sr;
                q.s.low = (n.s.high << (n_uword_bits - sr)) | (n.s.low >> sr);
                return q.all;
            }
            /* K X
             * ---
             * 0 K
             */
            sr = 1 + n_uword_bits + __builtin_clz(d.s.low) - __builtin_clz(n.s.high);
            /* 2 <= sr <= n_udword_bits - 1
             * q.all = n.all << (n_udword_bits - sr);
             * r.all = n.all >> sr;
             */
            if (sr == n_uword_bits)
            {
                q.s.low = 0;
                q.s.high = n.s.low;
                r.s.high = 0;
                r.s.low = n.s.high;
            }
            else if (sr < n_uword_bits)  // 2 <= sr <= n_uword_bits - 1
            {
                q.s.low = 0;
                q.s.high = n.s.low << (n_uword_bits - sr);
                r.s.high = n.s.high >> sr;
                r.s.low = (n.s.high << (n_uword_bits - sr)) | (n.s.low >> sr);
            }
            else              // n_uword_bits + 1 <= sr <= n_udword_bits - 1
            {
                q.s.low = n.s.low << (n_udword_bits - sr);
                q.s.high = (n.s.high << (n_udword_bits - sr)) |
                           (n.s.low >> (sr - n_uword_bits));
                r.s.high = 0;
                r.s.low = n.s.high >> (sr - n_uword_bits);
            }
        }
        else
        {
            /* K X
             * ---
             * K K
             */
            sr = __builtin_clz(d.s.high) - __builtin_clz(n.s.high);
            /* 0 <= sr <= n_uword_bits - 1 or sr large */
            if (sr > n_uword_bits - 1)
            {
                if (rem)
                    *rem = n.all;
                return 0;
            }
            ++sr;
            /* 1 <= sr <= n_uword_bits */
            /*  q.all = n.all << (n_udword_bits - sr); */
            q.s.low = 0;
            if (sr == n_uword_bits)
            {
                q.s.high = n.s.low;
                r.s.high = 0;
                r.s.low = n.s.high;
            }
            else
            {
                q.s.high = n.s.low << (n_uword_bits - sr);
                r.s.high = n.s.high >> sr;
                r.s.low = (n.s.high << (n_uword_bits - sr)) | (n.s.low >> sr);
            }
        }
    }
    /* Not a special case
     * q and r are initialized with:
     * q.all = n.all << (n_udword_bits - sr);
     * r.all = n.all >> sr;
     * 1 <= sr <= n_udword_bits - 1
     */
    su_int carry = 0;
    for (; sr > 0; --sr)
    {
        /* r:q = ((r:q)  << 1) | carry */
        r.s.high = (r.s.high << 1) | (r.s.low  >> (n_uword_bits - 1));
        r.s.low  = (r.s.low  << 1) | (q.s.high >> (n_uword_bits - 1));
        q.s.high = (q.s.high << 1) | (q.s.low  >> (n_uword_bits - 1));
        q.s.low  = (q.s.low  << 1) | carry;
        /* carry = 0;
         * if (r.all >= d.all)
         * {
         *      r.all -= d.all;
         *      carry = 1;
         * }
         */
        const di_int s = (di_int)(d.all - r.all - 1) >> (n_udword_bits - 1);
        carry = s & 1;
        r.all -= d.all & s;
    }
    q.all = (q.all << 1) | carry;
    if (rem)
        *rem = r.all;
    return q.all;
}
#endif
#if 0
asm(R"(
	.global __aeabi_uldivmod
		__aeabi_uldivmod:
        push	{r11, lr}
        sub	sp, sp, #16
        add	r12, sp, #8
        str	r12, [sp]
        bl	__udivmoddi4
        ldr	r2, [sp, #8]
        ldr	r3, [sp, #12]
        add	sp, sp, #16
        pop	{r11, pc}

	.global __aeabi_uidivmod
	__aeabi_uidivmod:
        push    { lr }
        sub     sp, sp, #4
        mov     r2, sp
        bl      __udivmodsi4
        ldr     r1, [sp]
        add     sp, sp, #4
        pop     { pc }
        )");
#endif
#if 0
R"(
	__udivmodsi4:
	str	r4, [sp, #-8]!

	mov	r4, r0
	adr	ip, div0block

	lsr	r3, r4, #16
	cmp	r3, r1
	movhs	r4, r3
	subhs	ip, ip, #(16 * 12)

	lsr	r3, r4, #8
	cmp	r3, r1
	movhs	r4, r3
	subhs	ip, ip, #(8 * 12)

	lsr	r3, r4, #4
	cmp	r3, r1
	movhs	r4, r3
	subhs	ip, #(4 * 12)

	lsr	r3, r4, #2
	cmp	r3, r1
	movhs	r4, r3
	subhs	ip, ip, #(2 * 12)

	/* Last block, no need to update r3 or r4. */
	cmp	r1, r4, lsr #1
	subls	ip, ip, #(1 * 12)

	ldr	r4, [sp], #8	/* restore r4, we are done with it. */
	mov	r3, #0

	bx ip
	)"

#define	IMM	#

#define block(shift) "cmp	r0, r1, lsl #"#shift "\n\t" \
	"addhs	r3, r3, #(1 <<" #shift ")\n\t"\
	"subhs	r0, r0, r1, lsl #" # shift "\n\t"

	block(31)
	block(30)
	block(29)
	block(28)
	block(27)
	block(26)
	block(25)
	block(24)
	block(23)
	block(22)
	block(21)
	block(20)
	block(19)
	block(18)
	block(17)
	block(16)
	block(15)
	block(14)
	block(13)
	block(12)
	block(11)
	block(10)
	block(9)
	block(8)
	block(7)
	block(6)
	block(5)
	block(4)
	block(3)
	block(2)
	block(1)
"div0block:"
	block(0)
	R"(
	str	r0, [r2]
	mov	r0, r3
	bx lr

quotient0:
	str	r0, [r2]
	mov	r0, #0
	bx lr

divby1:
	mov	r3, #0
	str	r3, [r2]
	bx lr

divby0:
	mov	r0, #0
	bx lr

)");
#endif
#ifndef __clang__
asm(R"(
.global __gnu_thumb1_case_uhi
.thumb_func
__gnu_thumb1_case_uhi:
	push    {r0, r1}
	mov     r1, lr
	lsr    r1, r1, #1
	lsl    r0, r0, #1
	lsl    r1, r1, #1
	ldrh    r1, [r1, r0]
	lsl    r1, r1, #1
	add     lr, lr, r1
	pop     {r0, r1}
	bx      lr
.global __gnu_thumb1_case_uqi
.thumb_func
__gnu_thumb1_case_uqi:
	mov     r12, r1
	mov     r1, lr
	lsr    r1, r1, #1
	lsl    r1, r1, #1
	ldrb    r1, [r1, r0]
	lsl    r1, r1, #1
	add     lr, lr, r1
	mov     r1, r12
	bx      lr
	)");
#endif
static int divide(int dividend, int divisor, int *rem)
{
   // int sign = ((dividend < 0) ^ (divisor < 0)) ? -1 : 1;
   // dividend = abs(dividend);
   // divisor = abs(divisor);
   if(divisor == 0) return 0xffffffff;
    int quotient = 0;
    while (dividend >= divisor) {
        dividend -= divisor;
        ++quotient;
    }
    if(rem) *rem = dividend;// * sign;
    return quotient;// * sign;
}
 
#if 0
unsigned __attribute__((used)) int __udivmodsi4(unsigned int divident, unsigned int divisor, unsigned int *remainder)
{
	return divide(divident, divisor, remainder);
}
unsigned __attribute__((used)) long long int __udivmoddi4(unsigned long long divident, unsigned long long divisor, unsigned long long *remainder)
{
	*remainder = 0;
	return divide(divident, divisor, (int*)remainder);
}
#endif

#define time(x) 11342
#endif
static void *last_brk;
static void *zcalloc(void *opaque, unsigned items, unsigned size)
{
	void *oldbrk;
	if(!last_brk)
		last_brk = (void*)brk( NULL );
	oldbrk = last_brk;
	last_brk = (void*)brk( (char*)last_brk + items * size );
	if( oldbrk == last_brk )
		return NULL;
	return oldbrk;
}
static void zcfree(void *opaque, void *ptr)
{
	// no free here, only run zip operations in forked processes!
}
