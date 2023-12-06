/* sunzip.c -- streaming unzip for reading a zip file from stdin
  Copyright (C) 2006, 2014, 2016, 2021 Mark Adler
  version 0.5  6 Jan 2021

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the author be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
	 claim that you wrote the original software. If you use this software
	 in a product, an acknowledgment in the product documentation would be
	 appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
	 misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Mark Adler    madler@alumni.caltech.edu
 */

/* Version history:
   0.1   3 Jun 2006  First version -- verifies deflated and stored entries
   0.2   4 Jun 2006  Add more PK signatures to reject or ignore
					 Allow for an Info-ZIP zip data descriptor signature
					 as well as a PKWare appnote data descriptor (no signature)
   0.3   4 Jul 2006  Handle (by skipping) digital sig and zip64 end fields
					 Use read() from stdin for speed (unbuffered)
					 Use inflateBack() instead of inflate() for speed
					 Handle deflate64 entries with inflateBack9()
					 Add quiet (-q) and really quiet (-qq) options
					 If stdin not redirected, give command help
					 Write out files, add -t option to just test
					 Add -o option to overwrite existing files
					 Decode and apply MS-DOS timestamp
					 Decode and apply Unix timestamp extra fields
					 Allow for several different types of data descriptors
					 Handle bzip2 (method 12) decompression
					 Process zip64 local headers and use full lengths
					 Use central directory for names to allow conversion
					 Apply external attributes from central directory
					 Detect and create symbolic links
					 Catch user interrupt and delete temporary junk
   0.31  7 Jul 2006  Get name from UTF-8 extra field if present
					 Fix zip64central offset bug
					 Change null-replacement character to underscore
					 Fix bad() error message mid-line handling
					 Fix stored length corruption bug
					 Verify that stored lengths are equal
					 Use larger input buffer when int type is large
					 Don't use calloc() when int type is large
   0.32 14 Jul 2006  Consolidate and simplify extra field processing
					 Use more portable stat() structure definitions
					 Allow use of mktemp() when mkdtemp() is not available
   0.33 23 Jul 2006  Replace futimes() with utimes() for portability
					 Fix bug in bzip2 decoding
					 Do two passes on command options to allow any order
					 Change pathbrk() return value to simplify usage
					 Protect against parent references ("..") in file names
					 Move name processing to after possibly getting UTF-8 name
   0.34 15 Jan 2014  Add option to change the replacement character for ..
					 Fix bug in the handling of extended timestamps
					 Allow bit 11 to be set in general purpose flags
   0.4  11 Jul 2016  Use blast for DCL imploded entries (method 10)
					 Add zlib license
   0.5   6 Jan 2021  Add -r option to retain temporary files in the event of
					 an error.

 */

/* Notes:
   - Compile and link sunzip with zlib 1.2.3 or later, infback9.c and
	 inftree9.c (found in the zlib source distribution in contrib/infback9),
	 blast.c from zlib 1.2.9 or later (found in contrib/blast), and libbzip2.
 */

/* To-do:
   - Set EIGHTDOT3 for file systems that so restrict the file names
   - Tailor path name operations for different operating systems
   - Set the long data descriptor signature once it's specified by PKWare
	 (looks like that will never happen)
   - Handle the entry name "-" differently?  (Created by piped zip.)
 */

/* ----- External Functions, Types, and Constants Definitions ----- */

#include <stdio.h>      /* printf(), fprintf(), fflush(), rename(), puts(), */
						/* fopen(), fread(), fclose() */
#include <stdlib.h>     /* exit(), malloc(), calloc(), free() */
#include <string.h>     /* memcpy(), strcpy(), strlen(), strcmp() */
#include <ctype.h>      /* tolower() */
#include <limits.h>     /* LONG_MIN */
#include <time.h>       /* mktime() */
#include <sys/time.h>   /* utimes() */
#include <assert.h>     /* assert() */
#include <signal.h>     /* signal() */
#include <unistd.h>     /* read(), close(), isatty(), chdir(), mkdtemp() or */
						/* mktemp(), unlink(), rmdir(), symlink() */
#include <fcntl.h>      /* open(), write(), O_WRONLY, O_CREAT, O_EXCL */
#include <sys/types.h>  /* for mkdir(), stat() */
#include <sys/stat.h>   /* mkdir(), stat() */
#include <errno.h>      /* errno, EEXIST */
#include <dirent.h>     /* opendir(), readdir(), closedir() */
#include "zlib.h"       /* crc32(), z_stream, inflateBackInit(), */
						/*   inflateBack(), inflateBackEnd() */
#ifndef JUST_DEFLATE
#include "infback9.h"   /* inflateBack9Init(), inflate9Back(), */
						/*   inflateBack9End() */
#include "blast.h"      /* blast() */
#include "bzlib.h"      /* BZ2_bzDecompressInit(), BZ2_bzDecompress(), */
						/*   BZ2_bzDecompressEnd() */
#endif

// dev branch stuff
#ifndef JUST_DEFLATE
void *sunalloc(void *opaque, unsigned items, unsigned size) {
	(void)opaque;
	return malloc(items * (size_t)size);
}
void sunfree(void *opaque, void *ptr) {
	(void)opaque;
	free(ptr);
}
#endif

#ifdef SUNZIP_TEST
typedef int sunzip_file_in;
typedef int sunzip_file_out; // fd
#define sunzip_out_valid(x) (x != -1)
#define sunzip_out_invalid (-1)
static inline int sunzip_write(sunzip_file_out file, const void *buf, size_t sz)
{
	return write( file, buf, sz );
}
static void create_directories(const char *path)
{
	const char *dir_begin = path, *dir_begin_next;
	char dir_path[PATH_MAX] = "";
	//size_t dir_len = 0;
	while((dir_begin_next = strchr(dir_begin, '/') ))
	{
		dir_begin_next++;
		memcpy(&dir_path[0] + (dir_begin - path), dir_begin, dir_begin_next - dir_begin);
		//printf("mkdir %s\n",dir_path);
		mkdir(dir_path, 0777);
		dir_begin = dir_begin_next;
	}
}
static inline sunzip_file_out sunzip_openout(const char *filename)
{
	create_directories(filename);
	return open(filename, O_WRONLY | O_CREAT, 0666);
}
static inline int sunzip_closeout(sunzip_file_out file)
{
	return close(file);
}

static inline int sunzip_read(sunzip_file_in file, void *buffer, size_t size)
{
	return read(file, buffer, size);
}
#define sunzip_printerr(...) fprintf(stderr, __VA_ARGS__)
#define sunzip_printout(...) fprintf(stderr, __VA_ARGS__)
#define sunzip_fatal() exit(1);
#else
#include "sunzip_integration.h"
#endif

/* ----- Language Readability Enhancements (sez me) ----- */

#define local static
#define until(c) while(!(c))

/* ----- Operating System Configuration and Tailoring ----- */

/* hack to avoid end-of-line conversions */
#if defined(MSDOS) || defined(OS2) || defined(WIN32) || defined(__CYGWIN__)
/* #  include <fcntl.h> */
#  include <io.h>
#  define SET_BINARY_MODE(file) setmode(file, O_BINARY)
#else
#  define SET_BINARY_MODE(file)
#endif

/* defines for the lengths of the integer types -- assure that longs are either
   four bytes or greater than or equal to eight bytes in length */
#if UINT_MAX > 0xffff
#  define BIGINT
#endif
#if ULONG_MAX >= 0xffffffffffffffffUL
#  define BIGLONG
#  if ULONG_MAX > 0xffffffffffffffffUL
#    define GIANTLONG
#  endif
#else
#  if ULONG_MAX != 0xffffffffUL
#    error Unexpected size of long data type
#  endif
#endif
#define NOCRC 1
#define SKIP_CENTRAL
/* systems for which mkdtemp() is not provided */
#ifdef VMS
#  define NOMKDTEMP
#endif

/* %% need to #define EIGHTDOT3 if limited to 8.3 names, e.g. DOS FAT */

/* ----- Operating System Specific Path Name Operations ----- */

/* %% This entire section should be tailored for various operating system
   conventions for path name syntax -- currently set up for Unix */

/* Safe file name character to replace nulls with */
#define SAFESEP '_'

/* Unix path delimiter */
#define PATHDELIM '/'

/* Unix parent reference and replacement character (repeated) */
#define PARENT ".."

static unsigned long entries;  /* number of entries seen */
/* abort with an error message */
local int bye(char *why)
{
	sunzip_printerr("sunzip abort: %s\n", why);
	sunzip_printerr("processed before error: %d\n", entries);
	sunzip_fatal();
	return 1;
}

/* ----- Input/Output Operations ----- */

/* structure for output processing */
struct out {
	sunzip_file_out file;                   /* output file or -1 to not write */
	unsigned long crc;          /* accumulated CRC-32 of output */
	unsigned long count;        /* output byte count */
	unsigned long count_hi;     /* count overflow */
};

/* process inflate output, writing if requested */
local int put(void *out_desc, unsigned char *buf, unsigned len)
{
	int wrote;
	unsigned try;
	struct out *out = (struct out *)out_desc;

#ifndef BIGINT
	/* handle special inflateBack9() case for 64K len */
	if (len == 0) {
		len = 32768U;
		put(out, buf, len);
		buf += len;
	}
#endif
#if !NOCRC
	/* update crc and output byte count */
	out->crc = crc32(out->crc, buf, len);
#endif
	out->count += len;
	if (out->count < len)
		out->count_hi++;
	if (sunzip_out_valid(out->file))
	{
		while (len) {   /* loop since write() may not complete request */
			try = len >= 32768U ? 16384 : len;
			wrote = sunzip_write(out->file, buf, try);
			if (wrote == -1)
				bye("write error");
			len -= wrote;
			buf += wrote;
		}
	}
	return 0;
}

/* structure for input acquisition and processing */
struct in {
	sunzip_file_in file;                   /* input file */
	unsigned char *buf;         /* input buffer */
	unsigned long count;        /* input byte count */
	unsigned long count_hi;     /* count overflow */
	unsigned long offset;       /* input stream offset of end of buffer */
	unsigned long offset_hi;    /* input stream offset overflow */
};

/* Input buffer size (must fit in signed int) */
#ifdef BIGINT
#  define CHUNK 131072
#else
#  define CHUNK 16384
#endif

/* Load input buffer, assumed to be empty, and return bytes loaded and a
   pointer to them.  read() is called until the buffer is full, or until it
   returns end-of-file or error.  Abort program on error using bye(). */
local unsigned get(void *in_desc, unsigned char **buf)
{
	int got;
	unsigned want, len;
	unsigned char *next;
	struct in *in = (struct in *)in_desc;

	next = in->buf;
	if (buf != NULL)
		*buf = next;
	want = CHUNK;
	do {        /* loop since read() not assured to return request */
		got = (int)sunzip_read(in->file, next, want);
		if (got == -1)
			bye("zip file read error");
		next += got;
		want -= got;
	} until (got == 0 || want == 0);
	len = CHUNK - want;         /* how much is in buffer */
	in->count += len;
	if (in->count < len)
		in->count_hi++;
	in->offset += len;
	if (in->offset < len)
		in->offset_hi++;
	return len;
}

/* load input buffer, abort if EOF */
#define load(in) ((left = get(in, NULL)) == 0 ? \
	bye("unexpected end of zip file") : (next = in->buf, left))

/* get one, two, or four bytes little-endian from the buffer, abort if EOF */
#define get1(in) (left == 0 ? load(in) : 0, left--, ++next, *(next-1))
#define get2(in) (tmp2 = get1(in), tmp2 + (get1(in) << 8))
#define get4(in) (tmp4 = get2(in), tmp4 + ((unsigned long)get2(in) << 16))

/* skip len bytes, abort if EOF */
#define skip(len, in) \
	do { \
		tmp4 = len; \
		while (tmp4 > left) { \
			tmp4 -= left; \
			load(in); \
		} \
		left -= (unsigned)tmp4; \
		next += (unsigned)tmp4; \
	} while (0)

/* read header field into output buffer */
#define field(len, in) \
	do { \
		tmp2 = len; \
		tmpp = outbuf; \
		while (tmp2 > left) { \
			memcpy(tmpp, next, left); \
			tmp2 -= left; \
			tmpp += left; \
			load(in); \
		} \
		memcpy(tmpp, next, tmp2); \
		left -= tmp2; \
		next += tmp2; \
	} while (0)

/* ----- File and Directory Operations ----- */

/* structure for directory cache, also saves times and pre-existence */
struct tree {
	char *name;             /* name of this directory */
	int new;                /* true if directory didn't already exist */
	long acc;               /* last access time */
	long mod;               /* last modification time */
	struct tree *subs;      /* list of subdirectories */
	struct tree *next;      /* next directory at this level */
};

/* ----- Zip Format Operations ----- */

/* pull two and four-byte little-endian integers from buffer */
#define little2(ptr) ((ptr)[0] + ((ptr)[1] << 8))
#define little4(ptr) (little2(ptr) + ((unsigned long)(little2(ptr + 2)) << 16))

/* find and return a specific extra block in an extra field */
local int getblock(unsigned id, unsigned char *extra, unsigned xlen,
				   unsigned char **block, unsigned *len)
{
	unsigned thisid, size;

	/* scan extra blocks */
	while (xlen) {
		/* get extra block id and data size */
		if (xlen < 4)
			return 0;               /* invalid block */
		thisid = little2(extra);
		size = little2(extra + 2);
		extra += 4;
		xlen -= 4;
		if (xlen < size)
			return 0;               /* invalid block */

		/* check for requested id */
		if (thisid == id) {
			*block = extra;
			*len = size;
			return 1;               /* got it! */
		}

		/* go to the next block */
		extra += size;
		xlen -= size;
	}
	return 0;                       /* wasn't there */
}


/* look for a zip64 block in the local header and update lengths, return
   true if got 8-byte lengths */
local int zip64local(unsigned char *extra, unsigned xlen,
					 unsigned long *clen, unsigned long *clen_hi,
					 unsigned long *ulen, unsigned long *ulen_hi)
{
	unsigned len;
	unsigned char *block;

	/* process zip64 Extended Information block */
	if (getblock(0x0001, extra, xlen, &block, &len) && len >= 16) {
		*ulen = little4(block);
		*ulen_hi = little4(block + 4);
		*clen = little4(block + 8);
		*clen_hi = little4(block + 12);
		return 1;           /* got 8-byte lengths */
	}
	return 0;               /* didn't get 8-byte lengths */
}

/* 32-bit marker for presence of 64-bit lengths */
#define LOW4 0xffffffffUL
#ifndef SKIP_CENTRAL
/* look for a zip64 block in the central header and update offset */
local void zip64central(unsigned char *extra, unsigned xlen,
						unsigned long clen, unsigned long ulen,
						unsigned long *offset, unsigned long *offset_hi)
{
	unsigned len;
	unsigned char *block;

	/* process zip64 Extended Information block */
	if (getblock(0x0001, extra, xlen, &block, &len) && len >= 16) {
		if (ulen == LOW4) {
			block += 8;
			len -= 8;
		}
		if (clen == LOW4) {
			block += 8;
			len -= 8;
		}
		if (len >= 8) {
			*offset = little4(block);
			*offset_hi = little4(block + 4);
		}
	}
}
#endif

#ifndef JUST_DEFLATE

/* ----- BZip2 Decompression Operation ----- */

#define BZOUTSIZE 32768U    /* passed outbuf better be this big */

/* decompress and write a bzip2 compressed entry */
local unsigned bunzip2(unsigned char *next, unsigned left,
					   struct in *in, struct out *out,
					   unsigned char *outbuf, unsigned char **back)
{
	int ret;
	bz_stream strm;

	/* initialize */
	strm.bzalloc = NULL;
	strm.bzfree = NULL;
	strm.opaque = NULL;
	ret = BZ2_bzDecompressInit(&strm, 0, 0);
	if (ret != BZ_OK)
		bye(ret == BZ_MEM_ERROR ? "out of memory" :
								  "internal error");

	/* decompress */
	strm.avail_in = left;
	strm.next_in = (char *)next;
	do {
		/* get more input if needed */
		if (strm.avail_in == 0) {
			strm.avail_in = get(in, NULL);
			if (strm.avail_in == 0)
				bye("unexpected end of zip file");
			strm.next_in = (char *)(in->buf);
		}

		/* process all of the buffered input */
		do {
			/* decompress to output buffer */
			strm.avail_out = BZOUTSIZE;
			strm.next_out = (char *)outbuf;
			ret = BZ2_bzDecompress(&strm);

			/* check for errors */
			switch (ret) {
			case BZ_MEM_ERROR:
				bye("out of memory");
				break;
			case BZ_DATA_ERROR:
			case BZ_DATA_ERROR_MAGIC:
				BZ2_bzDecompressEnd(&strm);
				*back = NULL;           /* return a compressed data error */
				return 0;
			case BZ_PARAM_ERROR:
				bye("internal error");
			}

			/* write out decompressed data */
			put(out, outbuf, BZOUTSIZE - strm.avail_out);

			/* repeat until output buffer not full (all input used) */
		} while (strm.avail_out == 0);

		/* go get more input and repeat until logical end of stream */
	} until (ret == BZ_STREAM_END);

	/* clean up and return unused input */
	BZ2_bzDecompressEnd(&strm);
	*back = (unsigned char *)(strm.next_in);
	return strm.avail_in;
}

#endif

/* display information about bad entry before aborting */
local void bad(char *why, unsigned long entry,
			   unsigned long here, unsigned long here_hi)
{
	sunzip_printerr("sunzip error: %s in entry #%lu at offset 0x", why, entry);
	if (here_hi)
		sunzip_printerr("%lx%08lx\n", here_hi, here);
	else
		sunzip_printerr("%lx\n", here);
}

/* macro to check actual crc and lengths against expected */
#ifdef BIGLONG
#  define GOOD() ((NOCRC || out->crc == crc) && \
	clen == (in->count & LOW4) && ulen == (out->count & LOW4) && \
	(high ? clen_hi == (in->count >> 32) && \
			ulen_hi == (out->count >> 32) : 1))
#else
#  define GOOD() ( (NOCRC || out->crc == crc) && \
	clen == in->count && ulen == out->count && \
	(high ? clen_hi == in->count_hi && \
			ulen_hi == out->count_hi : 1))
#endif

/* process a streaming zip file, i.e. without seeking: read input from file,
   limit output if quiet is 1, more so if quiet is >= 2, write the decompressed
   data to files if write is true, otherwise just verify the entries, overwrite
   existing files if over is true, otherwise don't -- over must not be true if
   write is false */
void sunzip(sunzip_file_in file, int write)
{
	enum {                      /* looking for ... */
		MARK,                   /* spanning signature (optional) */
		LOCAL,                  /* local headers */
		CENTRAL,                /* central directory headers */
		DIGSIG,                 /* digital signature (optional) */
		ZIP64REC,               /* zip64 end record (optional) */
		ZIP64LOC,               /* zip64 end locator (optional) */
		END,                    /* end record */
	} mode;                 /* current zip file mode */
	int ret = 0;            /* return value from zlib functions */
	int high;               /* true if have eight-byte length information */
	unsigned left;          /* bytes left in input buffer */
	//unsigned long exist;    /* how many already there so not written */
	unsigned flag;          /* general purpose flags from zip header */
	unsigned method;        /* compression method */
	unsigned nlen;          /* length of file name */
	unsigned xlen;          /* length of extra field */
	//unsigned madeby;        /* version and OS made by (in central directory) */
	unsigned tmp2;          /* temporary for get2() macro */
	unsigned long tmp4;     /* temporary for get4() and skip() macros */
	unsigned long here;     /* offset of this block */
	unsigned long here_hi;  /* high part of offset */
	unsigned long tmp;      /* temporary long */
	unsigned long crc;      /* cyclic redundancy check from header */
	unsigned long clen;     /* compressed length from header */
	unsigned long clen_hi;  /* high part of eight-byte compressed length */
	unsigned long ulen;     /* uncompressed length from header */
	unsigned long ulen_hi;  /* high part of eight-byte uncompressed length */
	//unsigned long extatt;   /* external file attributes */
   // long acc;               /* last access time for entry */
   // long mod;               /* last modified time for entry */
	unsigned char *tmpp;    /* temporary for field() macro */
	unsigned char *next;    /* pointer to next byte in input buffer */
	unsigned char *inbuf;   /* input buffer */
	unsigned char *outbuf;  /* output buffer and inflate window */
	//struct timeval times[2];            /* access and modify times */
	//struct stat st;                     /* for retrieving times */
	//FILE *sym;                          /* for reading symbolic link file */
	struct in ins, *in = &ins;          /* input structure */
	struct out outs, *out = &outs;      /* output structure */
	z_stream strms, *strm = NULL;       /* inflate structure */
#ifndef JUST_DEFLATE
	unsigned char *back;                /* returned next pointer */
	z_stream strms9, *strm9 = NULL;     /* inflate9 structure */
#endif
	char filepath[1024];
#ifdef BIGINT
	static int32_t inbuf_s[CHUNK / sizeof(int)], outbuf_s[16384];
#else
	static char inbuf_s[CHUNK], outbuf_s[65536];
#endif


	/* initialize i/o -- note that output buffer must be 64K both for
	   inflateBack9() as well as to load the maximum size name or extra
	   fields */
	outbuf = (unsigned char*) outbuf_s;
	inbuf = (unsigned char*) inbuf_s;

	left = 0;
	next = inbuf;
	in->file = file;
	in->buf = inbuf;
	in->offset = 0;
	in->offset_hi = 0;

	/* process zip file */
	mode = MARK;                /* start of zip file signature sequence */
	entries = 0;                /* entry count */
	do {
		/* mark current location */
		here = in->offset;
		here_hi = in->offset_hi;
		if (here < left)
			here_hi--;
		here -= left;

		/* get and interpret next header signature */
		switch (get4(in)) {

		case 0x08074b50UL:      /* spanning marker -- partial archive */
			if (mode != MARK)
				bye("zip file format error (spanning marker misplaced)");
			bye("cannot process split zip archives");
			break;

		case 0x30304b50UL:      /* non-split spanning marker (ignore) */
			if (mode != MARK)
				bye("zip file format error (spanning marker misplaced)");
			mode = LOCAL;
			break;

		case 0x04034b50UL:      /* local file header */
			if (mode > LOCAL)
				bye("zip file format error (local file header misplaced)");
			mode = LOCAL;
			entries++;

			/* process local header */
			(void)get2(in);                   /* version needed to extract */
			flag = get2(in);            /* general purpose flags */
			if ((flag & 9) == 9)
				bye("cannot skip encrypted entry with deferred lengths");
			if (flag & 0xf7f0U)
				bye("unknown zip header flags set");
			method = get2(in);          /* compression method */
			if ((flag & 8) && method != 8 && method != 9 && method != 12)
				bye("cannot handle deferred lengths for pre-deflate methods");
		   // acc = mod = dos2time(get4(in));     /* file date/time */
			(void)get4(in);
			crc = get4(in);             /* uncompressed CRC check value */
			clen = get4(in);            /* compressed size */
			clen_hi = 0;
			ulen = get4(in);            /* uncompressed size */
			ulen_hi = 0;
			high = 0;
			nlen = get2(in);            /* file name length */
			xlen = get2(in);            /* extra field length */

			/* skip file name (will get from central directory later) */
			field(nlen, in);
			memcpy(filepath, (char*)outbuf, nlen>1023?1023:nlen);
			filepath[nlen>1023?1023:nlen] = 0;
			/* process extra field -- get entry times if there and, if needed,
			   get zip64 lengths */
			field(xlen, in);            /* get extra field into outbuf */
			//xtimes(outbuf, xlen, &acc, &mod);
			if (!(flag & 8) && (clen == LOW4 || ulen == LOW4))
				high = zip64local(outbuf, xlen,
								  &clen, &clen_hi, &ulen, &ulen_hi);

			/* create temporary file (including for directories and links) */
			if (write && nlen && (filepath[nlen - 1] != PATHDELIM) && (method == 0 || method == 8 || method == 9 ||
						  method == 10 || method == 12)) {
				out->file = sunzip_openout(filepath);
				if (!sunzip_out_valid(out->file))
					bye("write error");
			}
			else
				out->file = sunzip_out_invalid;

			/* initialize crc, compressed, and uncompressed counts */
			in->count = left;
			in->count_hi = 0;
			out->count = 0;
			out->count_hi = 0;
			out->crc = crc32(0L, Z_NULL, 0);

			/* process compressed data */
			if (flag & 1)
				method = UINT_MAX;
			if (method == 0) {          /* stored */
				if (clen != ulen || clen_hi != ulen_hi)
					bye("zip file format error (stored lengths mismatch)");
				while (clen_hi || clen > left) {
					put(out, next, left);
					if (clen < left) {
						clen_hi--;
						clen = 0xffffffffUL - (left - clen - 1);
					}
					else
						clen -= left;
					load(in);
				}
				put(out, next, (unsigned)clen);
				left -= (unsigned)clen;
				next += (unsigned)clen;
				clen = ulen;
				clen_hi = ulen_hi;
			}
			else if (method == 8) {     /* deflated */
				if (strm == NULL) {     /* initialize inflater first time */
					strm = &strms;
					strm->zalloc = Z_NULL;
					strm->zfree = Z_NULL;
					strm->opaque = Z_NULL;
					ret = inflateBackInit(strm, 15, outbuf);
					if (ret != Z_OK)
						bye(ret == Z_MEM_ERROR ? "out of memory" :
												 "internal error");
				}
				strm->avail_in = left;
				strm->next_in = next;
				ret = inflateBack(strm, get, in, put, out);
				left = strm->avail_in;      /* reclaim unused input */
				next = strm->next_in;
				if (ret != Z_STREAM_END) {
					bad("deflate compressed data corrupted",
						entries, here, here_hi);
					bye("zip file corrupted -- cannot continue");
				}
			}
#ifndef JUST_DEFLATE
			else if (method == 9) {     /* deflated with deflate64 */
				if (strm9 == NULL) {    /* initialize first time */
					strm9 = &strms9;
					strm9->zalloc = Z_NULL;
					strm9->zfree = Z_NULL;
					strm9->opaque = Z_NULL;
					ret = inflateBack9Init(strm9, outbuf);
					if (ret != Z_OK)
						bye(ret == Z_MEM_ERROR ? "not enough memory (!)" :
												 "internal error");
				}
				strm9->avail_in = left;
				strm9->next_in = next;
				ret = inflateBack9(strm9, get, in, put, out);
				left = strm9->avail_in;      /* reclaim unused input */
				next = strm9->next_in;
				if (ret != Z_STREAM_END) {
					bad("deflate64 compressed data corrupted",
						entries, here, here_hi);
					bye("zip file corrupted -- cannot continue");
				}
			}
			else if (method == 10) {    /* PKWare DCL implode */
				ret = blast(get, in, put, out, &left, &next);
				if (ret != 0) {
					bad("DCL imploded data corrupted",
						entries, here, here_hi);
					bye("zip file corrupted -- cannot continue");
				}
			}
			else if (method == 12) {    /* bzip2 compression */
				left = bunzip2(next, left, in, out, outbuf, &back);
				if (back == NULL) {
					bad("bzip2 compressed data corrupted",
						entries, here, here_hi);
					bye("zip file corrupted -- cannot continue");
				}
				next = back;
			}
#endif
			else {                      /* skip encrpyted or unknown method */
					bad(flag & 1 ? "skipping encrypted entry" :
						"skipping unknown compression method",
						entries, here, here_hi);
				skip(clen, in);
				tmp = clen_hi;
				while (tmp) {
					skip(0x80000000UL, in);
					skip(0x80000000UL, in);
					tmp--;
				}
			}

			/* deduct unused input from compressed data count */
			if (in->count < left)
				in->count_hi--;
			in->count -= left;

			/* close file, set file times */
			if (sunzip_out_valid(out->file)) {
				if (sunzip_closeout(out->file))
					bye("write error");
			  /*  times[0].tv_sec = acc;
				times[0].tv_usec = 0;
				times[1].tv_sec = mod;
				times[1].tv_usec = 0;
				utimes(tempdir, times);*/
			}

			/* get data descriptor if present --
			   allow for several possibilities: four-byte or eight-byte
			   lengths, with no signature or with one of two signatures (the
			   second signature is not known yet -- to be defined by PKWare --
			   for now allow only one), note that this will not be attempted
			   for skipped entries, since skipped entries cannot have deferred
			   lengths */
			if (flag & 8) {
				/* look for PKWare descriptor (even though no one uses it?) */
				crc = get4(in);         /* uncompressed data check value */
				clen = get4(in);        /* compressed size */
				clen_hi = 0;
				ulen = get4(in);        /* uncompressed size */
				ulen_hi = 0;
				if (!GOOD()) {
					/* look for an Info-ZIP descriptor (original -- in use) */
					/* (%% NOTE: replace second signature when actual known) */
					if (crc == 0x08074b50UL || crc == 0x08074b50UL) {
						tmp = crc;      /* temporary hold for signature */
						crc = clen;
						clen = ulen;
						ulen = get4(in);
						if (!GOOD()) {
							/* try no signature with eight-byte lengths */
							clen_hi = clen;
							clen = crc;
							crc = tmp;
							ulen_hi = get4(in);
							high = 1;
							if (!GOOD()) {
								/* try signature with eight-byte lengths */
								crc = clen;
								clen = clen_hi;
								clen_hi = ulen;
								ulen = ulen_hi;
								ulen_hi = get4(in);
							}
						}
					}
					else {
						/* try no signature with eight-byte lengths */
						clen_hi = ulen;
						ulen = get4(in);
						ulen_hi = get4(in);
						high = 1;
					}
				}
			}

			/* verify entry and display information (won't do if skipped) */
			if (method == 0 || method == 8 || method == 9 || method == 10 ||
				method == 12) {
				if (!GOOD()) {
					bad("compressed data corrupted, check values mismatch",
						entries, here, here_hi);
					//bye("zip file corrupted -- cannot continue");
				}
			}
			break;

		case 0x02014b50UL:      /* central file header */
			/* first time here: any earlier mode can arrive here */
			if (mode < CENTRAL) {
					sunzip_printout("%lu entr%s processed\n",
						   entries, entries == 1 ? "y" : "ies");
				mode = CENTRAL;
			}
#ifndef SKIP_CENTRAL
			/* read central header */
			if (mode != CENTRAL)
				bye("zip file format error (central file header misplaced)");
			(void)get1(in);                   /* version made by */
			(void)get1(in);                   /* OS made by */
			skip(14, in);               /* skip up through crc */
			clen = get4(in);            /* compressed length */
			ulen = get4(in);            /* uncompressed length */
			nlen = get2(in);            /* file name length */
			xlen = get2(in);            /* extra field length */
			flag = get2(in);            /* comment length */
			skip(4, in);                /* disk #, internal attributes */
			get4(in);                   /* external attributes */
			here = get4(in);            /* offset of local header */
			here_hi = 0;

			/* get and save file name, compute name crc */
			skip(nlen, in);                /* get file name */
			/* process extra field to get 64-bit offset, if there */
			field(xlen, in);                /* get extra field */
			zip64central(outbuf, xlen, clen, ulen, &here, &here_hi);
#ifdef BIGLONG
			here += here_hi << 32;
			here_hi = 0;
#endif

			/* skip comment field -- last thing in central header */
			skip(flag, in);
#else
			skip(24, in);
			nlen = get2(in);
			xlen = get2(in);
			flag = get2(in);
			skip(12 + nlen + xlen + flag, in);
#endif
			break;

		case 0x05054b50UL:      /* digital signature */
			if (mode != CENTRAL)
				bye("zip file format error (digital signature misplaced)");
			mode = DIGSIG;
			skip(get2(in), in);
			break;

		case 0x06064b50UL:      /* zip64 end of central directory record */
			if (mode != CENTRAL && mode != DIGSIG)
				bye("zip file format error (zip64 record misplaced)");
			mode = ZIP64REC;
			ulen = get4(in);
			ulen_hi = get4(in);
			skip(ulen, in);
			while (ulen_hi) {           /* truly odd, but possible */
				skip(0x80000000UL, in);
				skip(0x80000000UL, in);
				ulen_hi--;
			}
			break;

		case 0x07064b50UL:      /* zip64 end of central directory locator */
			if (mode != ZIP64REC)
				bye("zip file format error (zip64 locator misplaced)");
			mode = ZIP64LOC;
			skip(16, in);
			break;

		case 0x06054b50UL:      /* end of central directory record */
			if (mode == LOCAL || mode == ZIP64REC || mode == END)
				bye("zip file format error (end record misplaced)");
			mode = END;
			skip(16, in);               /* counts and offsets */
			flag = get2(in);            /* zip file comment length */
			skip(flag, in);             /* zip file comment */
			break;

		default:
			sunzip_printerr("bad signature 0x%x at %lu, mode %d\n", tmp4, in->offset, mode );
			bye("zip file format error (unknown zip signature)");
		}
	} until (mode == END);              /* until end record reached (or EOF) */

#ifndef JUST_DEFLATE
	if (strm9 != NULL)
		inflateBack9End(strm9);
#endif
	if (strm != NULL)
		inflateBackEnd(strm);

	/* check for junk */
	if (left != 0 || get(in, NULL) != 0) {
		fflush(stdout);
		fputs("sunzip warning: junk after end of zip file\n", stderr);
	}
}
#ifdef SUNZIP_TEST
/* process arguments and then unzip from stdin */
int main(int argc, char **argv)
{
	(void)argc, (void)argv;
	/* unzip from stdin */

	SET_BINARY_MODE(0);      /* for defective operating systems */

	sunzip(0, 1);
	return 0;
}
#endif
