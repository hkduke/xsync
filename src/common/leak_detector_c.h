/**
 * leak_detector_c.h - for gcc
 *   http://www.codeproject.com/Articles/19361/Memory-Leak-Detection-in-C
 *
 * place below code at where you want to detect:
 *   atexit(report_mem_leak_file);
 *   atexit(report_mem_leak_stdout);
 *
 * or:
 *   USE_REPORT_LEAK_FILE();
 *   USE_REPORT_LEAK_STDOUT();
 */
#ifndef  LEAK_DETECTOR_C_H
#define  LEAK_DETECTOR_C_H

#include  <stdio.h>
#include  <stdlib.h>
#include  <malloc.h>
#include  <string.h>
#include  <time.h>

#define  LEAK_FNAME_SIZE  256
#define  LEAK_REPORT_FILE        "/home/leak-info.rpt"

#define USE_REPORT_LEAK_FILE()    atexit(report_mem_leak_file)
#define USE_REPORT_LEAK_STDOUT()  atexit(report_mem_leak_stdout)

#define  malloc(size)              xmalloc (size, __FILE__, __LINE__)
#define  calloc(elements, size)    xcalloc (elements, size, __FILE__, __LINE__)
#define  free(mem_ref)             xfree(mem_ref)

#ifndef uint32_t
  typedef unsigned int uint32_t;
#endif

typedef struct _MEM_INFO {
  void      *addr;
  uint32_t   size;
  char       fname[LEAK_FNAME_SIZE];
  uint32_t   line;
} MEM_INFO;

typedef struct _MEM_LEAK {
  MEM_INFO mem_info;
  struct _MEM_LEAK *next;
} MEM_LEAK;

void add(MEM_INFO alloc_info);
void erase(unsigned pos);
void clear(void);

void * xmalloc(uint32_t size, const char * file, uint32_t line);
void * xcalloc(uint32_t elements, uint32_t size, const char * file, uint32_t line);
void xfree(void * mem_ref);

void add_mem_info (void * mem_ref, uint32_t size, const char * file, uint32_t line);
void remove_mem_info (void * mem_ref);
void report_mem_leak_file(void);
void report_mem_leak_stdout(void);

#endif /* leak_detector_c.h */
