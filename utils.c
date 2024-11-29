/* The MIT License

   Copyright (c) 2018-     Dana-Farber Cancer Institute
                 2009-2018 Broad Institute, Inc.
                 2008-2009 Genome Research Ltd. (GRL)

   Permission is hereby granted, free of charge, to any person obtaining
   a copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, sublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be
   included in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
   BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
   ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
   SOFTWARE.
*/
#define FSYNC_ON_FLUSH

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#ifdef FSYNC_ON_FLUSH
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#endif
#include <sys/resource.h>
#include <sys/time.h>
#include "utils.h"

#include "ksort.h"
#define pair64_lt(a, b) ((a).x < (b).x || ((a).x == (b).x && (a).y < (b).y))
KSORT_INIT(128, pair64_t, pair64_lt)
KSORT_INIT(64,  uint64_t, ks_lt_generic)

#include "kseq.h"
//KSEQ_INIT2(, gzFile, err_gzread)
KSEQ_INIT2(, int, my_align_read)

/********************
 * System utilities *
 ********************/

FILE *err_xopen_core(const char *func, const char *fn, const char *mode)
{
	FILE *fp = 0;
	if (strcmp(fn, "-") == 0)
		return (strstr(mode, "r"))? stdin : stdout;
	if ((fp = fopen(fn, mode)) == 0) {
		err_fatal(func, "fail to open file '%s' : %s", fn, strerror(errno));
	}
	return fp;
}

FILE *err_xreopen_core(const char *func, const char *fn, const char *mode, FILE *fp)
{
	if (freopen(fn, mode, fp) == 0) {
		err_fatal(func, "fail to open file '%s' : %s", fn, strerror(errno));
	}
	return fp;
}

gzFile err_xzopen_core(const char *func, const char *fn, const char *mode)
{
	gzFile fp;
	if (strcmp(fn, "-") == 0) {
		fp = gzdopen(fileno((strstr(mode, "r"))? stdin : stdout), mode);
		/* According to zlib.h, this is the only reason gzdopen can fail */
		if (!fp) err_fatal(func, "Out of memory");
		return fp;
	}
	if ((fp = gzopen(fn, mode)) == 0) {
		err_fatal(func, "fail to open file '%s' : %s", fn, errno ? strerror(errno) : "Out of memory");
	}
	return fp;
}

void err_fatal(const char *header, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "[%s] ", header);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, "\n");
	va_end(args);
	exit(EXIT_FAILURE);
}

void err_fatal_core(const char *header, const char *fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "[%s] ", header);
	vfprintf(stderr, fmt, args);
	fprintf(stderr, " Abort!\n");
	va_end(args);
	abort();
}

void _err_fatal_simple(const char *func, const char *msg)
{
	fprintf(stderr, "[%s] %s\n", func, msg);
	exit(EXIT_FAILURE);
}

void _err_fatal_simple_core(const char *func, const char *msg)
{
	fprintf(stderr, "[%s] %s Abort!\n", func, msg);
	abort();
}

size_t err_fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = fwrite(ptr, size, nmemb, stream);
	if (ret != nmemb) 
		_err_fatal_simple("fwrite", strerror(errno));
	return ret;
}

size_t err_fread_noeof(void *ptr, size_t size, size_t nmemb, FILE *stream)
{
	size_t ret = fread(ptr, size, nmemb, stream);
	if (ret != nmemb)
	{
		_err_fatal_simple("fread", ferror(stream) ? strerror(errno) : "Unexpected end of file");
	}
	return ret;
}

extern double t_step1_read;

int my_align_read(int file, void *ptr, unsigned int len)
{
    double t0 = realtime();
	int ret = read(file, ptr, len);
    t_step1_read += realtime() - t0;

	if (ret < 0)
	{
		int errnum = 0;
        const char *msg = strerror(errno);
		_err_fatal_simple("my_align_read", Z_ERRNO == errnum ? strerror(errno) : msg);
	}

	return ret;
}

int err_gzread(gzFile file, void *ptr, unsigned int len)
{
	int ret = gzread(file, ptr, len);

	if (ret < 0)
	{
		int errnum = 0;
		const char *msg = gzerror(file, &errnum);
		_err_fatal_simple("gzread", Z_ERRNO == errnum ? strerror(errno) : msg);
	}

	return ret;
}

int err_fseek(FILE *stream, long offset, int whence)
{
	int ret = fseek(stream, offset, whence);
	if (0 != ret)
	{
		_err_fatal_simple("fseek", strerror(errno));
	}
	return ret;
}

long err_ftell(FILE *stream)
{
	long ret = ftell(stream);
	if (-1 == ret)
	{
		_err_fatal_simple("ftell", strerror(errno));
	}
	return ret;
}

int err_printf(const char *format, ...) 
{
	va_list arg;
	int done;
	va_start(arg, format);
	done = vfprintf(stdout, format, arg);
	int saveErrno = errno;
	va_end(arg);
	if (done < 0) _err_fatal_simple("vfprintf(stdout)", strerror(saveErrno));
	return done;
}

int err_fprintf(FILE *stream, const char *format, ...) 
{
	va_list arg;
	int done;
	va_start(arg, format);
	done = vfprintf(stream, format, arg);
	int saveErrno = errno;
	va_end(arg);
	if (done < 0) _err_fatal_simple("vfprintf", strerror(saveErrno));
	return done;
}

int err_fputc(int c, FILE *stream)
{
	int ret = putc(c, stream);
	if (EOF == ret)
	{
		_err_fatal_simple("fputc", strerror(errno));
	}

	return ret;
}

int err_fputs(const char *s, FILE *stream)
{
	int ret = fputs(s, stream);
	if (EOF == ret)
	{
		_err_fatal_simple("fputs", strerror(errno));
	}

	return ret;
}


#define BS_size (4 << 20)

//char align_write_buf[BS_size] __attribute__((aligned(4096)));
//int buf_used_size = 0;

void my_align_write(char *s, int out_fd, int tag, char *out_file) {

    static int buf_used_size = 0;
    static int cntt = 0;
    cntt++;
    static char* align_write_buf;
    
    if(cntt == 1) {
        align_write_buf = (unsigned char*)aligned_alloc(4096, BS_size);
    }

    if(tag == 1) {
        close(out_fd);
        if (buf_used_size > 0) {
            int new_fd = open(out_file, O_CREAT | O_WRONLY | O_APPEND, 0666);
            if (new_fd == -1) {
                perror("Failed to open file for appending");
                return;
            }
            int ret = write(new_fd, align_write_buf, buf_used_size);
            if (ret == -1) {
                perror("Write failed");
                close(new_fd);
                return;
            }
            buf_used_size = 0;
            close(new_fd);
        }
        return;
    }

    size_t len = strlen(s);

    while (len > 0) {
       
        size_t space_left = BS_size - buf_used_size;

        size_t to_copy = (len < space_left) ? len : space_left;

        memcpy(align_write_buf + buf_used_size, s, to_copy);
        //printf("len %d, space_left %d, to_copy %d, buf_used_size %d\n", len, space_left, to_copy, buf_used_size);

        buf_used_size += to_copy;
        s += to_copy;
        len -= to_copy;

        if (buf_used_size == BS_size) {
            //printf("output chunk to %d\n", out_fd);
            write(out_fd, align_write_buf, BS_size);
            buf_used_size = 0;
        }
    }
}

//void flush_align_write(int out_fd) {
//    if (buf_used_size > 0) {
//        write(out_fd, align_write_buf, buf_used_size);
//        buf_used_size = 0;
//    }
//}

int err_puts(const char *s)
{
	int ret = puts(s);
	if (EOF == ret)
	{
		_err_fatal_simple("puts", strerror(errno));
	}

	return ret;
}

int err_fflush(FILE *stream) 
{
    int ret = fflush(stream);
    if (ret != 0) _err_fatal_simple("fflush", strerror(errno));

#ifdef FSYNC_ON_FLUSH
	/* Calling fflush() ensures that all the data has made it to the
	   kernel buffers, but this may not be sufficient for remote filesystems
	   (e.g. NFS, lustre) as an error may still occur while the kernel
	   is copying the buffered data to the file server.  To be sure of
	   catching these errors, we need to call fsync() on the file
	   descriptor, but only if it is a regular file.  */
	{
		struct stat sbuf;
		if (0 != fstat(fileno(stream), &sbuf))
			_err_fatal_simple("fstat", strerror(errno));
		
		if (S_ISREG(sbuf.st_mode))
		{
			if (0 != fsync(fileno(stream)))
				_err_fatal_simple("fsync", strerror(errno));
		}
	}
#endif
    return ret;
}

int err_fclose(FILE *stream) 
{
	int ret = fclose(stream);
	if (ret != 0) _err_fatal_simple("fclose", strerror(errno));
	return ret;
}

int err_gzclose(gzFile file)
{
	int ret = gzclose(file);
	if (Z_OK != ret)
	{
		_err_fatal_simple("gzclose", Z_ERRNO == ret ? strerror(errno) : zError(ret));
	}

	return ret;
}

/*********
 * Timer *
 *********/

double cputime(void)
{
	struct rusage r;
	getrusage(RUSAGE_SELF, &r);
	return r.ru_utime.tv_sec + r.ru_stime.tv_sec + 1e-6 * (r.ru_utime.tv_usec + r.ru_stime.tv_usec);
}

double realtime(void)
{
	struct timeval tp;
	struct timezone tzp;
	gettimeofday(&tp, &tzp);
	return tp.tv_sec + tp.tv_usec * 1e-6;
}

long peakrss(void)
{
	struct rusage r;
	getrusage(RUSAGE_SELF, &r);
#ifdef __linux__
	return r.ru_maxrss * 1024;
#else
	return r.ru_maxrss;
#endif
}
