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
#ifndef JS_SPACE_GC_THREASHOLD
#ifdef GC_COPY
#define JS_SPACE_GC_THREASHOLD     (JS_SPACE_BYTES >> 3)
#else
#define JS_SPACE_GC_THREASHOLD     (JS_SPACE_BYTES >> 2)
#endif
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

struct space;

/*
 * variables
 */
STATIC struct space js_space;
#ifdef GC_DEBUG
STATIC struct space debug_js_shadow;
#endif /* GC_DEBUG */

/* old gc root stack (to be obsolete) */
#define MAX_TMP_ROOTS 1000
STATIC JSValue *tmp_roots[MAX_TMP_ROOTS];
STATIC int tmp_roots_sp;

/* new gc root stack */
#define MAX_ROOTS 1000
STATIC JSValue *gc_root_stack[MAX_ROOTS];
STATIC int gc_root_stack_ptr = 0;

#ifdef GC_IS_MOVING_GC
/* regbase stack */
#define MAX_REGBASES 32
STATIC JSValue **gc_regbase_stack[MAX_REGBASES];
STATIC int gc_regbase_stack_ptr = 0;
#endif /* GC_IS_MOVING_GC */

STATIC int gc_disabled = 1;

int generation = 0;
time_t gc_sec;
suseconds_t gc_usec; 
time_t gc_sec_max;
suseconds_t gc_usec_max; 

#ifdef GC_DEBUG
STATIC void **top;
STATIC void sanity_check();
#endif /* GC_DEBUG */

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

STATIC AllocProfile gc_alloc_profiles[(1 << HEADER_BYTES)];

STATIC size_t moved_object_bytes;
STATIC size_t moved_object_count;
STATIC size_t scaned_object_count;
#endif /* GC_PROFILE */

/*
 * prototype
 */
/* space */
STATIC void create_space(struct space *space, size_t bytes, char* name);
STATIC int in_js_space(void *addr_);
#ifdef GC_DEBUG
STATIC HeaderCell *get_shadow(void *ptr);
#endif /* GC_DEBUG */
STATIC void* space_alloc(struct space *space,
                         size_t request_bytes, cell_type_t type);

/* GC */
STATIC int check_gc_request(Context *);
STATIC void garbage_collect(Context *ctx);
STATIC void collect(Context *ctx);

#ifdef GC_IS_MOVING_GC
STATIC void copy_object(void *from, void *to, size_t num_jsvalues);
#endif /* GC_IS_MOVING_GC */

#ifdef GC_DEBUG
STATIC void check_invariant(void);
STATIC void print_memory_status(void);
STATIC void print_heap_stat(void);
#endif /* GC_DEBUG */
#if (defined GC_DEBUG) || (defined GC_PROFILE)
STATIC const char *get_name_HTAG(cell_type_t htag);
#endif /* (defined GC_DEBUG) || (defined GC_PROFILE) */

#ifdef GC_CLEAR_MEM
STATIC void fill_free_cell(struct space *p, JSValue val);
#endif /* GC_CLEAR_MEM */

#ifdef GC_PROFILE
STATIC void add_doubled_size(struct doubled_size *pdsize, size_t size);
STATIC void print_doubled_size(struct doubled_size *pdsize);
STATIC void regist_alloc_profile(cell_type_t type, size_t request, size_t allocate, size_t header, size_t waste);
STATIC void print_profile(cell_type_t type);
STATIC size_t gc_get_allocated_bytes();
#endif /* GC_PROFILE */

/*
 * Implementation
 */
#if (defined GC_MARK_SWEEP)
#include "gc_marksweep.c"
#elif (defined GC_MARK_COMPACT)
#include "gc_markcompact.c"
#elif (defined GC_THREADED_COMPACT)
#include "gc_threadedcompact.c"
#elif (defined GC_COPY)
#include "gc_copy.c"
#elif
#error "Please define macro to select GC algorithm : GC_MARK_SWEEP / GC_MARK_COMPACT / GC_THREADED_COMPACT / GC_COPY"
#endif

/*
 * GC
 */

void init_memory()
{
  create_space(&js_space, JS_SPACE_BYTES, "js_space");
#ifdef GC_DEBUG
  create_space(&debug_js_shadow, JS_SPACE_BYTES, "debug_js_shadow");
#endif /* GC_DEBUG */
  tmp_roots_sp = -1;
  gc_root_stack_ptr = 0;
  gc_disabled = 0;
  generation = 1;
  gc_sec = 0;
  gc_usec = 0;
  gc_sec_max = 0;
  gc_usec_max = 0;
}

void gc_push_tmp_root(JSValue *loc)
{
  tmp_roots[++tmp_roots_sp] = loc;
}

void gc_push_tmp_root2(JSValue *loc1, JSValue *loc2)
{
  tmp_roots[++tmp_roots_sp] = loc1;
  tmp_roots[++tmp_roots_sp] = loc2;
}

void gc_push_tmp_root3(JSValue *loc1, JSValue *loc2, JSValue *loc3)
{
  tmp_roots[++tmp_roots_sp] = loc1;
  tmp_roots[++tmp_roots_sp] = loc2;
  tmp_roots[++tmp_roots_sp] = loc3;
}

void gc_pop_tmp_root(int n)
{
  tmp_roots_sp -= n;
}

void gc_push_checked(void *addr)
{
  gc_root_stack[gc_root_stack_ptr++] = (JSValue *) addr;
}

void gc_pop_checked(void *addr)
{
#ifdef GC_DEBUG
  if (gc_root_stack[gc_root_stack_ptr - 1] != (JSValue *) addr) {
    fprintf(stderr, "GC_POP pointer does not match\n");
    abort();
  }
#endif /* GC_DEBUG */
  gc_root_stack[--gc_root_stack_ptr] = NULL;
}

#ifdef GC_IS_MOVING_GC
void gc_push_regbase(JSValue **pregbase)
{
  gc_regbase_stack[gc_regbase_stack_ptr++] = pregbase;
}

void gc_pop_regbase(JSValue **pregbase)
{
#ifdef GC_DEBUG
  if (gc_regbase_stack[gc_regbase_stack_ptr - 1] != pregbase) {
    fprintf(stderr, "GC_POP_REGBASE pointer does not match\n");
    abort();
  }
#endif /* GC_DEBUG */
  gc_regbase_stack[--gc_regbase_stack_ptr] = NULL;
}
#endif /* GC_IS_MOVING_GC */

cell_type_t gc_obj_header_type(void *p)
{
  HeaderCell *hdrp = VALPTR_TO_HEADERPTR(p);
  return HEADER_GET_TYPE(hdrp);
}

STATIC int check_gc_request(Context *ctx)
{
  if (ctx == NULL) {
    if (js_space.free_bytes < JS_SPACE_GC_THREASHOLD)
      GCLOG_TRIGGER("Needed gc for js_space -- cancelled: ctx == NULL\n");
    return 0;
  }
  if (gc_disabled) {
    if (js_space.free_bytes < JS_SPACE_GC_THREASHOLD)
      GCLOG_TRIGGER("Needed gc for js_space -- cancelled: GC disabled\n");
    return 0;
  }
  if (js_space.free_bytes < JS_SPACE_GC_THREASHOLD)
    return 1;
  GCLOG_TRIGGER("no GC needed (%d bytes free)\n", js_space.free_bytes);
  return 0;
}

void* gc_malloc(Context *ctx, uintptr_t request_bytes, uint32_t type)
{
  return gc_jsalloc(ctx, request_bytes, type);
}

JSValue* gc_jsalloc(Context *ctx, uintptr_t request_bytes, uint32_t type)
{
  JSValue *addr;
#ifdef GC_DEBUG
  top = (void**) &ctx;
#endif /* GC_DEBUG */

  if (check_gc_request(ctx))
    garbage_collect(ctx);
  addr = space_alloc(&js_space, request_bytes, type);
  GCLOG_ALLOC("gc_jsalloc: req %x bytes type %d => %p\n",
              request_bytes, type, addr);
#ifdef GC_DEBUG
  {
  HeaderCell *hdrp = VALPTR_TO_HEADERPTR(addr);
  HeaderCell *shadow = get_shadow(hdrp);
  *shadow = *hdrp;
  }
#endif /* GC_DEBUG */
#ifdef GC_PROFILE
  {
    size_t allocate = HEADER_GET_SIZE(VALPTR_TO_HEADERPTR(addr)) << LOG_BYTES_IN_JSVALUE;
    size_t header = HEADER_BYTES;
    size_t waste = allocate - header - request_bytes;
    regist_alloc_profile(type, request_bytes, allocate, header, waste);
  }
#endif /* GC_PROFILE */
  return addr;
}

void disable_gc(void)
{
  gc_disabled++;
}

void enable_gc(Context *ctx)
{
#ifdef GC_DEBUG
  top = (void**) &ctx;
#endif /* GC_DEBUG */

  if (--gc_disabled == 0) {
    if (check_gc_request(ctx))
      garbage_collect(ctx);
  }
}

void try_gc(Context *ctx)
{
  if (check_gc_request(ctx))
    garbage_collect(ctx);
}

STATIC void garbage_collect(Context *ctx)
{
  struct rusage ru0, ru1;
#ifdef GC_PROFILE
  size_t allocated_bytes_old, allocated_bytes_new;
#endif /* GC_PROFILE */

  /* printf("Enter gc, generation = %d\n", generation); */
  GCLOG("Before Garbage Collection\n");
  /* print_memory_status(); */
  if (cputime_flag == TRUE) getrusage(RUSAGE_SELF, &ru0);

#ifdef GC_PROFILE
  allocated_bytes_old = gc_get_allocated_bytes();
  moved_object_bytes = 0;
  moved_object_count = 0;
  scaned_object_count = 0;
#endif /* GC_PROFILE */

  collect(ctx);

#ifdef GC_PROFILE
  allocated_bytes_new = gc_get_allocated_bytes();
  if (collectinfo_flag == TRUE) {
    size_t collect_bytes;
    double collect_rate;

    collect_bytes = (size_t) (allocated_bytes_old - allocated_bytes_new);
    collect_rate = (double) collect_bytes / (double) allocated_bytes_old * 100.0;

    printf("allocated bytes : %zu to %zu : %zu bytes ( %.2f %%) collected\n", allocated_bytes_old, allocated_bytes_new, collect_bytes, collect_rate);
  }
  if (movinginfo_flag == TRUE) {
    double moving_count_rate, moving_byte_rate;

    moving_count_rate = (double) moved_object_count / (double) scaned_object_count * 100.0;
    moving_byte_rate = (double) moved_object_bytes / (double) allocated_bytes_new * 100.0;
    printf("%zu / %zu ( %.2f %%) objects moved : %zu bytes ( %.2f %%)\n", moved_object_count, scaned_object_count, moving_count_rate, moved_object_bytes, moving_byte_rate);
  }
#endif /* GC_PROFILE */

#ifdef GC_CLEAR_MEM
  fill_free_cell(&js_space, (JSValue)GC_CLEAR_MEM);
#endif
  GCLOG("After Garbage Collection\n");
  /* print_memory_status(); */
  /* print_heap_stat(); */

  if (cputime_flag == TRUE) {
    time_t sec;
    suseconds_t usec;

    getrusage(RUSAGE_SELF, &ru1);
    sec = ru1.ru_utime.tv_sec - ru0.ru_utime.tv_sec;
    usec = ru1.ru_utime.tv_usec - ru0.ru_utime.tv_usec;
    if (usec < 0) {
      sec--;
      usec += 1000000;
    }
    gc_sec += sec;
    gc_usec += usec;
    if (gc_usec >= 1000000) {
      gc_usec -= 1000000;
      ++gc_sec;
    }

    if (sec > gc_sec_max || (sec == gc_sec_max && usec > gc_usec_max)) {
      gc_sec_max = sec;
      gc_usec_max = usec;
    }
#ifdef GC_TIMEOUT
    if (gc_sec >= GC_TIMEOUT_SEC && gc_usec >= GC_TIMEOUT_USEC) {
      printf("GC time out = %ld.%03d msec, max GC time = %ld.%03d msec (#GC = %d)\n",
            gc_sec * 1000 + gc_usec / 1000, (int)(gc_usec % 1000),
            gc_sec_max * 1000 + gc_usec_max / 1000, (int)(gc_usec_max % 1000), generation);
      abort();
      return;
    }
#endif /* GC_TIMEOUT_SEC */
  }

  generation++;
  /* printf("Exit gc, generation = %d\n", generation); */
}

#ifdef GC_IS_MOVING_GC
STATIC void copy_object(void *from, void *to, size_t num_jsvalues)
{
  JSValue *pfrom, *pend, *pto;

  pfrom = (JSValue *) from;
  pto = (JSValue *) to;

#ifdef GC_PROFILE
  ++scaned_object_count;
#endif /* GC_PROFILE */

  if (pfrom == pto) return;

#ifdef GC_PROFILE
  moved_object_bytes += num_jsvalues << LOG_BYTES_IN_JSVALUE;
  ++moved_object_count;
#endif /* GC_PROFILE */

  pend = pfrom + num_jsvalues;
  while (pfrom < pend) {
    *pto = *pfrom;
    ++pto;
    ++pfrom;
  }
}
#endif /* GC_IS_MOVING_GC */

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
