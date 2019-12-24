/*
 * eJS Project
 * Kochi University of Technology
 * The University of Electro-communications
 *
 * The eJS Project is the successor of the SSJS Project at The University of
 * Electro-communications.
 */

#include <stdlib.h>
#include <stdio.h>
#include "prefix.h"
#define EXTERN extern
#include "header.h"
#include "log.h"

/*
 * GC Options
 * 
 * GC_DEBUG : enable debug code
 * 
 * GC_NULL             : null gc
 * GC_MARK_SWEEP       : mark & sweep gc
 * GC_MARK_COMPACT     : mark & compaction gc
 * GC_THREADED_COMPACT : mark & threaded compaction gc
 * GC_COPY             : copy gc
 * 
 * GC_CLEAR_MEM : clear unused memory with this value
 * 
 * GC_TIMEOUT      : enable timeout of gc
 * GC_TIMEOUT_SEC  : timeout second
 * GC_TIMEOUT_USEC : timeout usec
 * 
 * GC_PROFILE : enable gc profiling
 */

#ifndef NDEBUG
#define GC_DEBUG 1
#define STATIC
#else
#undef GC_DEBUG
#define STATIC static
#endif

/*
 * #define GCLOG(...) LOG(__VA_ARGS__)
 * #define GCLOG_TRIGGER(...) LOG(__VA_ARGS__)
 * #define GCLOG_ALLOC(...) LOG(__VA_ARGS__)
 * #define GCLOG_SWEEP(...) LOG(__VA_ARGS__)
 */

#define GCLOG(...)
#define GCLOG_TRIGGER(...)
#define GCLOG_ALLOC(...)
#define GCLOG_SWEEP(...)


/*
 * naming convention
 *   name for size: add a surfix representing the unit
 *                    bytes: in bytes
 *                    jsvalues: in the numberof JSValue's
 */

#ifndef JS_SPACE_BYTES
#define JS_SPACE_BYTES     (10 * 1024 * 1024)
#endif

#include "cell-header.h"

/*
 *  Macro
 */

#define HEADER_JSVALUES ((HEADER_BYTES + BYTES_IN_JSVALUE - 1) >> LOG_BYTES_IN_JSVALUE)

#if (defined GC_TIMEOUT_SEC) || (defined GC_TIMEOUT_USEC)
#define GC_TIMEOUT
#if (defined GC_TIMEOUT_SEC) && !(defined GC_TIMEOUT_USEC)
#define GC_TIMEOUT_USEC 0
#endif /* (defined GC_TIMEOUT_SEC) && !(defined GC_TIMEOUT_USEC) */
#if !(defined GC_TIMEOUT_SEC) && (defined GC_TIMEOUT_USEC)
#define GC_TIMEOUT_SEC 0
#endif /* !(defined GC_TIMEOUT_SEC) && (defined GC_TIMEOUT_USEC) */
#endif /* (defined GC_TIMEOUT_SEC) || (defined GC_TIMEOUT_USEC) */

/*
 *  Types
 */

#define HTAG_FREE ((1 << HEADER_TYPE_BITS) - 1)

struct space { };

/*
 * variables
 */
STATIC struct space js_space;

int generation = 0;
time_t gc_sec;
suseconds_t gc_usec; 
time_t gc_sec_max;
suseconds_t gc_usec_max; 

#ifdef GC_PROFILE
struct doubled_size {
  size_t low;  /* count lower 10 digits */
  size_t high; /* count higher 10 digits */
};

typedef struct alloc_profile {
  int count;
  struct doubled_size request;
  struct doubled_size allocate;
  struct doubled_size header;
  struct doubled_size waste;
} AllocProfile;

STATIC AllocProfile gc_alloc_profiles[(1 << HEADER_TYPE_BITS)];

STATIC size_t moved_object_bytes;
STATIC size_t moved_object_count;
STATIC size_t scaned_object_count;
#endif /* GC_PROFILE */

/*
 * prototype
 */
/* space */
STATIC void* space_alloc(struct space *space,
                         size_t request_bytes, cell_type_t type);

#ifdef GC_PROFILE
STATIC void add_doubled_size(struct doubled_size *pdsize, size_t size);
STATIC void print_doubled_size(struct doubled_size *pdsize);
STATIC void regist_alloc_profile(cell_type_t type, size_t request, size_t allocate, size_t header, size_t waste);
STATIC void print_profile(cell_type_t type);
STATIC size_t gc_get_allocated_bytes();
#endif /* GC_PROFILE */


/*
 * GC
 */

void init_memory() { }

cell_type_t gc_obj_header_type(void *p)
{
  HeaderCell *hdrp = VALPTR_TO_HEADERPTR(p);
  return HEADER_GET_TYPE(hdrp);
}

void* gc_malloc(Context *ctx, uintptr_t request_bytes, uint32_t type) { return NULL; }

JSValue* gc_jsalloc(Context *ctx, uintptr_t request_bytes, uint32_t type) { return NULL; }

STATIC void* space_alloc(struct space *space,
                         size_t request_bytes, cell_type_t type) { return NULL; }

#if (defined GC_DEBUG) || (defined GC_PROFILE)
STATIC const char *get_name_HTAG(cell_type_t htag)
{
  switch(htag) {
      case HTAG_FREE: return "FREE";

      case HTAG_STRING: return "STRING";
      case HTAG_FLONUM: return "FLONUM";
      case HTAG_SIMPLE_OBJECT: return "OBJECT";
      case HTAG_ARRAY: return "ARRAY";
      case HTAG_FUNCTION: return "FUNCTION";
      case HTAG_BUILTIN: return "BUILTIN";
      case HTAG_ITERATOR: return "ITERATOR";
#ifdef use_regexp
      case HTAG_REGEXP: return "REGEXP";
#endif
      case HTAG_BOXED_STRING: return "BOX_STRING";
      case HTAG_BOXED_NUMBER: return "BOX_NUMBER";
      case HTAG_BOXED_BOOLEAN: return "BOX_BOOLEAN";


      case HTAG_PROP: return "PROP";
      case HTAG_ARRAY_DATA: return "ARRAY_DATA";
      case HTAG_FUNCTION_FRAME: return "FUNCTION_FRAME";
      case HTAG_HASH_BODY: return "HASH_BODY";
      case HTAG_STR_CONS: return "STR_CONS";
      case HTAG_CONTEXT: return "CONTEXT";
      case HTAG_STACK: return "STACK";
      case HTAG_HIDDEN_CLASS: return "HIDDEN_CLASS";
      
      default: return "UNKNOWN";
  }
}
#endif /* (defined GC_DEBUG) || (defined GC_PROFILE) */

#ifdef GC_PROFILE
STATIC size_t gc_get_allocated_bytes()
{
  return (size_t) 0;
}

#define GC_PROFILE_SIZE_BORDER 1000000000

STATIC void add_doubled_size(struct doubled_size *pdsize, size_t size)
{
  while (size >= GC_PROFILE_SIZE_BORDER) {
    size -= GC_PROFILE_SIZE_BORDER;
    ++(pdsize->high);
  }
  if (pdsize->low > GC_PROFILE_SIZE_BORDER - size) { /* low + size > GC_PROFILE_SIZE_BORDER */
    pdsize->low = pdsize->low - (GC_PROFILE_SIZE_BORDER - size); /* low + size - GC_PROFILE_SIZE_BORDER */
    ++(pdsize->high);
  }
  else {
    pdsize->low += size;
  }
}

STATIC void print_doubled_size(struct doubled_size *pdsize)
{
  if (pdsize->high == 0) printf("%ld", pdsize->low);
  else printf("%ld%010ld", pdsize->high, pdsize->low);
}

STATIC void regist_alloc_profile(cell_type_t type, size_t request, size_t allocate, size_t header, size_t waste)
{
  AllocProfile *p;

  p = &(gc_alloc_profiles[type]);

  ++(p->count);
  add_doubled_size(&(p->request), request);
  add_doubled_size(&(p->allocate), allocate);
  add_doubled_size(&(p->header), header);
  add_doubled_size(&(p->waste), waste);
}

STATIC void print_profile(cell_type_t type) {
  AllocProfile *p;

  p = &(gc_alloc_profiles[type]);
  printf("type %s : allocated %d times, ", get_name_HTAG(type), p->count);
  printf("request ");   print_doubled_size(&(p->request));  printf(" bytes, ");
  printf("allocated "); print_doubled_size(&(p->allocate)); printf(" bytes, ");
  printf("header ");    print_doubled_size(&(p->header));   printf(" bytes, ");
  printf("waste ");     print_doubled_size(&(p->waste));    printf(" bytes\n");
}

void print_gc_alloc_profile()
{
  print_profile(HTAG_STRING);
  print_profile(HTAG_FLONUM);
  print_profile(HTAG_SIMPLE_OBJECT);
  print_profile(HTAG_ARRAY);
  print_profile(HTAG_FUNCTION);
  print_profile(HTAG_BUILTIN);
  print_profile(HTAG_ITERATOR);
  print_profile(HTAG_BOXED_STRING);
  print_profile(HTAG_BOXED_NUMBER);
  print_profile(HTAG_BOXED_BOOLEAN);
  print_profile(HTAG_PROP);
  print_profile(HTAG_ARRAY_DATA);
  print_profile(HTAG_FUNCTION_FRAME);
  print_profile(HTAG_HASH_BODY);
  print_profile(HTAG_STR_CONS);
  print_profile(HTAG_STACK);
  print_profile(HTAG_HIDDEN_CLASS);
}
#endif /* GC_PROFILE */

/* Local Variables:      */
/* mode: c               */
/* c-basic-offset: 2     */
/* indent-tabs-mode: nil */
/* End:                  */
