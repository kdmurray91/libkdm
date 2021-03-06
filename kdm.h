/*
 * ============================================================================
 *
 *       Filename:  kdm.h
 *
 *    Description:  libkdm: header-only utility to make C more DRY-ish
 *
 *        Version:  1.0
 *        Created:  03/03/14 17:06:23
 *       Revision:  none
 *        License:  GPLv3+
 *       Compiler:  gcc / clang
 *
 *         Author:  Kevin Murray, spam@kdmurray.id.au
 *
 * ============================================================================
 */

#ifndef KDM_H
#define KDM_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


/*
 * Cross-platform bollocks. Thanks windows.
 */

#if defined(WIN32) || defined(_WIN32)
#include <windows.h>
#define km_pathsep "\\"
#else
#define km_pathsep "/"
#include <unistd.h>
#endif

/*
 * Macro helpers from tor
 */

/* Expands to a syntactically valid empty statement.  */
#define STMT_NIL (void)0

/* Expands to a syntactically valid empty statement, explicitly (void)ing its
 * argument. */
#define STMT_VOID(a) while (0) { (void)(a); }

#ifdef __GNUC__
/* STMT_BEGIN and STMT_END are used to wrap blocks inside macros so that
 * the macro can be used as if it were a single C statement. */
#define STMT_BEGIN (void) ({
#define STMT_END })
#elif defined(sun) || defined(__sun__)
#define STMT_BEGIN if (1) {
#define STMT_END } else STMT_NIL
#else
#define STMT_BEGIN do {
#define STMT_END } while (0)
#endif


/*
 * General helper macros
 */
#define km_likely(x)      __builtin_expect(!!(x), 1)
#define km_unlikely(x)    __builtin_expect(!!(x), 0)


/*
 * Error handling constants
 */

#define KDM_ERR_ALLOC 1<<0
#define KDM_ERR_FREE 1<<1

static const char *km_err_msgs[] = {
    "No Error",
    "Could not allocate memory",
    "Could not free memory",
    "Null pointer passed to function expecting valid memory address",
    "Bad path passed to function expecting valid filesystem path",
    NULL
};


/*
 * Error handling functions
 */

static void (*km_exit)(int) = &exit;

/* Valid non-function to pass to libkdm functions */
static void
km_onerr_nil(int err, char *msg, char *file,  int line)
{
    (void) (err);
    (void) (msg);
    (void) (file);
    (void) (line);
}

/* Function to pass to libkdm functions which prints out errors to stderr */
static void
km_onerr_print (int err, char *msg,  char *file, int line)
{
    if (msg == NULL) {
        fprintf(stderr, "[%s: %d] %d: %s\n", file, line, err,
                km_err_msgs[err]);
    } else {
        fprintf(stderr, "[%s: %d] %d: %s -- %s\n", file, line, err,
                km_err_msgs[err], msg);
    }
}

/* Function to pass to libkdm functions which prints out errors to stderr and
   calls `exit(EXIT_FAILURE)` */
static void
km_onerr_print_exit (int err, char *msg,  char *file, int line)
{
    km_onerr_print(err, msg, file, line);
    (*km_exit)(EXIT_FAILURE);
}

/*
 * Memory allocation/deallocation
 */

static inline void *
km_calloc_ (size_t n, size_t size, void (*onerr)(int, char *, char *, int),
            char *file, int line)
{
    void *ret = calloc(n, size);
    if (ret == NULL) {
        (*onerr)(1, NULL, file, line);
        return NULL;
    } else {
        return ret;
    }
}
#define km_calloc(n, sz, fn) km_calloc_(n, sz, fn, __FILE__, __LINE__)

static inline void *
km_malloc_ (size_t size, void (*onerr)(int, char *, char *, int),
            char *file, int line)
{
    void *ret = malloc(size);
    if (ret == NULL) {
        (*onerr)(1, NULL, file, line);
        return NULL;
    } else {
        return ret;
    }
}
#define km_malloc(sz, fn) km_malloc_(sz, fn, __FILE__, __LINE__)

static inline void *
km_realloc_ (void *data, size_t size, void (*onerr)(int, char *, char *, int),
             char *file, int line)
{
    void *ret = realloc(data, size);
    if (ret == NULL) {
        (*onerr)(1, NULL, file, line);
        return NULL;
    } else {
        return ret;
    }
}
#define km_realloc(ptr, sz, fn) km_realloc_(ptr, sz, fn, __FILE__, __LINE__)

#define km_free(data)               \
    STMT_BEGIN                      \
    if (data != NULL) {             \
        free(data);                 \
        data = NULL;                \
    }                               \
    STMT_END

/*
 * Bit fiddling and maths hacks
 */


/* Flogged from http://stackoverflow.com/a/1322548 and
   http://graphics.stanford.edu/~seander/bithacks.html, and kseq.h */
/* Round a 32-bit int up to nearest base-2 number */
#define	kmroundup32(v) (((v) & ((v) - 1)) == 0) ? /* Power of 2 */ \
        (                               \
            (v)|=(v)>>1,                \
            (v)|=(v)>>2,                \
            (v)|=(v)>>4,                \
            (v)|=(v)>>8,                \
            (v)|=(v)>>16,               \
            ++(v)                       \
        ) : (                           \
            --(v),                      \
            (v)|=(v)>>1,                \
            (v)|=(v)>>2,                \
            (v)|=(v)>>4,                \
            (v)|=(v)>>8,                \
            (v)|=(v)>>16,               \
            ++(v)                       \
        )

/* Round a 64-bit int up to nearest base-2 number */
#define	kmroundup64(v) (((v) & ((v) - 1)) == 0) ? /* Power of 2 */ \
        (                               \
            (v)|=(v)>>1,                \
            (v)|=(v)>>2,                \
            (v)|=(v)>>4,                \
            (v)|=(v)>>8,                \
            (v)|=(v)>>16,               \
            (v)|=(v)>>32,               \
            ++(v)                       \
        ) : (                           \
            --(v),                      \
            (v)|=(v)>>1,                \
            (v)|=(v)>>2,                \
            (v)|=(v)>>4,                \
            (v)|=(v)>>8,                \
            (v)|=(v)>>16,               \
            (v)|=(v)>>32,               \
            ++(v)                       \
        )

#if UINTPTR_MAX == 0xffffffffffffffff
#define kmroundupz kmroundup64
#elif UINTPTR_MAX == 0xffffffff
#define kmroundupz kmroundup32
#endif

/*
 * IO routines
 */

/* Zero-copy `getline()`, with the added benefit of `realloc`-ing `buf` to
    the next highest base-2 power, if we run out of space. If it is realloced,
    (*size) is updated to the right size.
    DON'T USE ON STACK BUFFERS.
*/
static inline ssize_t
km_readline_realloc_ (char **buf, FILE *fp, size_t *size,
              void (*onerr)(int, char *, char *, int), char *file, int line)
{
    if (buf == NULL || (*buf) == NULL || fp == NULL || size == NULL) {
        (*onerr)(3, NULL, file, line);
        return -2; /* EOF is normally == -1, so use -2 to differentiate them */
    }
    size_t len = 0;
    while(((*buf)[len] = fgetc(fp)) != EOF && (*buf)[len++] != '\n') {
        while (len + 1 >= (*size) - 1) {
            size_t newsize = (*size) + 1;
            *size = kmroundupz(newsize);
            char *newbuf = km_realloc(*buf, sizeof(**buf) * (*size), onerr);
            if (newbuf == NULL) {
                return -2;
            } else (*buf) = newbuf;
        }
    }
    (*buf)[len] = '\0';
    if (feof(fp)) return EOF;
    else return len;
}
#define km_readline_realloc(buf, fp, sz, fn) \
    km_readline_realloc_(buf, fp, sz, fn, __FILE__, __LINE__)

#endif /* KDM_H */
