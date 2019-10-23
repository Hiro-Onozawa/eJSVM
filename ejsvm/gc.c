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
 * GC_MARK_SWEEP   : mark & sweep gc
 * GC_MARK_COMPACT : mark & compation gc
 * 
 * GC_CLEAR_MEM : clear unused memory with this value
 */

#ifndef NDEBUG
#define GC_DEBUG 1
#define STATIC
#else
#undef GC_DEBUG
#define STATIC static
#endif

#if (!defined GC_MARK_SWEEP) && (!defined GC_MARK_COMPACT)
#error "Please define macro to select GC algorithm : GC_MARK_SWEEP / GC_MARK_COMPACT"
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
#define JS_SPACE_GC_THREASHOLD     (JS_SPACE_BYTES >> 1)
/* #define JS_SPACE_GC_THREASHOLD     (JS_SPACE_BYTES >> 4) */

/*
 * If the remaining room is smaller than a certain size,
 * we do not use the remainder for efficiency.  Rather,
 * we add it below the chunk being allocated.  In this case,
 * the size in the header includes the extra words.
 */
#define MINIMUM_FREE_CHUNK_JSVALUES 4

#include "cell-header.h"

/*
 *  Macro
 */

#define GC_MARK_BIT (1 << HEADER_GC_OFFSET)
#define HEADER_JSVALUES ((HEADER_BYTES + BYTES_IN_JSVALUE - 1) >> LOG_BYTES_IN_JSVALUE)

/*
 *  Types
 */

#define HTAG_FREE ((1 << HEADER_TYPE_BITS) - 1)

struct free_chunk {
  HeaderCell header;
  struct free_chunk *next;
};

struct space {
  uintptr_t addr;
  size_t bytes;
  size_t free_bytes;
  struct free_chunk* freelist;
  char *name;
};

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

/* regbase stack */
#define MAX_REGBASES 32
STATIC JSValue **gc_regbase_stack[MAX_REGBASES];
STATIC int gc_regbase_stack_ptr = 0;

STATIC int gc_disabled = 1;

int generation = 0;
int gc_sec;
int gc_usec; 

#ifdef GC_DEBUG
STATIC void **top;
STATIC void sanity_check();
#endif /* GC_DEBUG */

/*
 * prototype
 */
/* space */
STATIC void create_space(struct space *space, size_t bytes, char* name);
STATIC int in_js_space(void *addr_);
#ifdef GC_DEBUG
STATIC HeaderCell *get_shadow(void *ptr);
#endif /* GC_DEBUG */
/* GC */
STATIC int check_gc_request(Context *);
STATIC void garbage_collect(Context *ctx);

#if (defined GC_MARK_SWEEP) || (defined GC_MARK_COMPACT)
STATIC void trace_HashCell_array(HashCell ***ptrp, uint32_t length);
STATIC void trace_HashCell(HashCell **ptrp);
#ifdef HIDDEN_CLASS
STATIC void trace_HiddenClass(HiddenClass **ptrp);
#endif /* HIDDEN_CLASS */
STATIC void trace_JSValue_array(JSValue **ptrp, size_t length);
STATIC void trace_slot(JSValue* ptr);
STATIC void scan_roots(Context *ctx);
STATIC void scan_stack(JSValue* stack, int sp, int fp);
STATIC void weak_clear_StrTable(StrTable *table);
STATIC void weak_clear(void);
STATIC void sweep(void);

#ifdef GC_DEBUG
STATIC void check_invariant(void);
STATIC void print_memory_status(void);
STATIC void print_heap_stat(void);
#endif /* GC_DEBUG */

#ifdef GC_MARK_COMPACT
STATIC void compaction(Context *ctx);
STATIC void set_fwdptr(void);
STATIC void update_roots(Context *ctx);
STATIC void update_regbase(size_t diff);
STATIC void move_heap_object(void);
#endif /* GC_MARK_COMPACT */
#endif /* (defined GC_MARK_SWEEP) || (defined GC_MARK_COMPACT) */

#ifdef GC_CLEAR_MEM
STATIC void fill_free_cell(struct free_chunk *p, JSValue val);
#endif /* GC_CLEAR_MEM */

/*
 *  Space
 */

STATIC void create_space(struct space *space, size_t bytes, char *name)
{
  struct free_chunk *p;
  p = (struct free_chunk *) malloc(bytes);
  p->header.header0 = HEADER0_COMPOSE(bytes >> LOG_BYTES_IN_JSVALUE, 0, HTAG_FREE);
#ifdef GC_DEBUG
  HEADER_SET_MAGIC(&p->header, HEADER_MAGIC);
#endif /* GC_DEBUG */
  p->next = NULL;
  space->addr = (uintptr_t) p;
  space->bytes = bytes;
  space->free_bytes = bytes;
  space->freelist = p;
  space->name = name;
}

STATIC int in_js_space(void *addr_)
{
  uintptr_t addr = (uintptr_t) addr_;
  return (js_space.addr <= addr && addr < js_space.addr + js_space.bytes);
}

#ifdef GC_DEBUG
STATIC HeaderCell *get_shadow(void *ptr)
{
  if (in_js_space(ptr)) {
    uintptr_t a = (uintptr_t) ptr;
    uintptr_t off = a - js_space.addr;
    return (HeaderCell *) (debug_js_shadow.addr + off);
  } else
    return NULL;
}
#endif /* GC_DEBUG */

/*
 * Returns a pointer to the first address of the memory area
 * available to the VM.  The header precedes the area.
 * The header has the size of the chunk including the header,
 * the area available to the VM, and extra bytes if any.
 * Other header bits are zero
 */
STATIC void* space_alloc(struct space *space,
                         size_t request_bytes, cell_type_t type)
{
  size_t  alloc_jsvalues;
  struct free_chunk **p;
  
  alloc_jsvalues =
    (request_bytes + BYTES_IN_JSVALUE - 1) >> LOG_BYTES_IN_JSVALUE;
  alloc_jsvalues += HEADER_JSVALUES;

  /* allocate from freelist */
  for (p = &space->freelist; *p != NULL; p = &(*p)->next) {
    struct free_chunk *chunk = *p;
    size_t chunk_jsvalues = HEADER_GET_SIZE(&chunk->header);
    if (chunk_jsvalues >= alloc_jsvalues) {
      if (chunk_jsvalues >= alloc_jsvalues + MINIMUM_FREE_CHUNK_JSVALUES) {
        /* This chunk is large enough to leave a part unused.  Split it */
        size_t new_chunk_jsvalues = chunk_jsvalues - alloc_jsvalues;
        uintptr_t addr =
          ((uintptr_t) chunk) + (new_chunk_jsvalues << LOG_BYTES_IN_JSVALUE);
        HEADER_SET_SIZE(&chunk->header, new_chunk_jsvalues);
        ((HeaderCell *) addr)->header0 = HEADER0_COMPOSE(alloc_jsvalues, 0, type);
        ((HeaderCell *) addr)->header1 = 0;
#ifdef GC_DEBUG
        HEADER_SET_MAGIC((HeaderCell *) addr, HEADER_MAGIC);
        HEADER_SET_GEN_MASK((HeaderCell *) addr, generation);
#endif /* GC_DEBUG */
        space->free_bytes -= alloc_jsvalues << LOG_BYTES_IN_JSVALUE;
        return HEADERPTR_TO_VALPTR(addr);
      } else {
        /* This chunk is too small to split. */
        *p = (*p)->next;
        chunk->header.header0 =
          HEADER0_COMPOSE(chunk_jsvalues,
                          chunk_jsvalues - alloc_jsvalues, type);
        chunk->header.header1 = 0;
#ifdef GC_DEBUG
        HEADER_SET_MAGIC(&chunk->header, HEADER_MAGIC);
        HEADER_SET_GEN_MASK(&chunk->header, generation);
#endif /* GC_DEBUG */
        space->free_bytes -= chunk_jsvalues << LOG_BYTES_IN_JSVALUE;
        return HEADERPTR_TO_VALPTR(chunk);
      }
    }
  }

  printf("memory exhausted\n");
  return NULL;
}


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

  /* printf("Enter gc, generation = %d\n", generation); */
  GCLOG("Before Garbage Collection\n");
  /* print_memory_status(); */
  if (cputime_flag == TRUE) getrusage(RUSAGE_SELF, &ru0);

#ifdef GC_MARK_SWEEP
  scan_roots(ctx);
  weak_clear();
  sweep();
#endif
#ifdef GC_MARK_COMPACT
  scan_roots(ctx);
  weak_clear();
  sweep();
  compaction(ctx);
#endif
#ifdef GC_CLEAR_MEM
  fill_free_cell(js_space.freelist, (JSValue)GC_CLEAR_MEM);
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
  }

  generation++;
  /* printf("Exit gc, generation = %d\n", generation); */
}

#if (defined GC_MARK_SWEEP) || (defined GC_MARK_COMPACT)
/*
 * Mark the header
 */
STATIC void mark_cell_header(HeaderCell *hdrp)
{
#ifdef GC_DEBUG
  {
    HeaderCell *shadow = get_shadow(hdrp);
    assert(HEADER_GET_MAGIC(hdrp) == HEADER_MAGIC);
    assert(HEADER_GET_TYPE(hdrp) == HEADER_GET_TYPE(shadow));
    assert(HEADER_GET_SIZE(hdrp) - HEADER_GET_EXTRA(hdrp) ==
           HEADER_GET_SIZE(shadow) - HEADER_GET_EXTRA(shadow));
    assert(HEADER_GET_GEN(hdrp) == HEADER_GET_GEN(shadow));
  }
#endif /* GC_DEBUG */
  HEADER_GC_GET_WORD(hdrp) |= GC_MARK_BIT;
}

STATIC void mark_cell(void *ref)
{
  HeaderCell *hdrp = VALPTR_TO_HEADERPTR(ref);
  mark_cell_header(hdrp);
}

STATIC void unmark_cell_header(HeaderCell *hdrp)
{
  HEADER_GC_GET_WORD(hdrp) &= ~GC_MARK_BIT;
}

STATIC int is_marked_cell_header(HeaderCell *hdrp)
{
#if HEADER_GC_OFFSET <= 4 * 8  /* BITS_IN_INT */
  return HEADER_GC_GET_WORD(hdrp) & GC_MARK_BIT;
#else
  return !!(HEADER_GC_GET_WORD(hdrp) & GC_MARK_BIT);
#endif
}

STATIC int is_marked_cell(void *ref)
{
  HeaderCell *hdrp = VALPTR_TO_HEADERPTR(ref);
  return is_marked_cell_header(hdrp);
}

STATIC int test_and_mark_cell(void *ref)
{
  if (in_js_space(ref)) {
    HeaderCell *hdrp = VALPTR_TO_HEADERPTR(ref);
    if (is_marked_cell_header(hdrp))
      return 1;
    mark_cell_header(hdrp);
  }
  return 0;
}

/*
 * Tracer
 */

STATIC void trace_leaf_object(uintptr_t *ptrp)
{
  uintptr_t ptr = *ptrp;
  if (in_js_space((void *) ptr))
    mark_cell((void *) ptr);
}

STATIC void trace_HashTable(HashTable **ptrp)
{
  HashTable *ptr = *ptrp;

  if (test_and_mark_cell(ptr))
    return;

  trace_HashCell_array(&ptr->body, ptr->size);
}

STATIC void trace_HashCell_array(HashCell ***ptrp, uint32_t length)
{
  HashCell **ptr = *ptrp;
  int i;
  if (test_and_mark_cell(ptr))
    return;

  for (i = 0; i < length; i++) {
    if (ptr[i] != NULL)
      trace_HashCell(ptr + i);
  }
}

STATIC void trace_HashCell(HashCell **ptrp)
{
  HashCell *ptr = *ptrp;
  if (test_and_mark_cell(ptr))
    return;

  trace_slot(&ptr->entry.key);
#ifdef HIDDEN_CLASS
  if (is_transition(ptr->entry.attr))
    trace_HiddenClass((HiddenClass **)&ptr->entry.data);
#endif
  if (ptr->next != NULL)
    trace_HashCell(&ptr->next);
}

STATIC void trace_Instruction_array_part(Instruction **ptrp,
                                         size_t n_insns, size_t n_constants)
{
  Instruction *ptr = (Instruction *) *ptrp;
  JSValue *litstart;
  size_t i;
  if (test_and_mark_cell(ptr))
    return;
  litstart = (JSValue *)(&ptr[n_insns]);
  for (i = 0; i < n_constants; i++)
    trace_slot((JSValue *)(&litstart[i]));
}

STATIC void scan_FunctionTable(FunctionTable *ptr)
{
  /* trace constant pool */
  trace_Instruction_array_part(&ptr->insns, ptr->n_insns, ptr->n_constants);
}

STATIC void trace_FunctionTable_array(FunctionTable **ptrp, size_t length)
{
  FunctionTable *ptr = *ptrp;
  size_t i;
  if (test_and_mark_cell(ptr))
    return;
  for (i = 0; i < length; i++)
    scan_FunctionTable(ptr + i);
}

STATIC void trace_FunctionFrame(FunctionFrame **ptrp)
{
  FunctionFrame *ptr = *ptrp;
  header_word_t header;
  size_t length;
  size_t   i;
  if (test_and_mark_cell(ptr))
    return;

  if (ptr->prev_frame != NULL)
    trace_FunctionFrame(&ptr->prev_frame);
  trace_slot(&ptr->arguments);
  /* locals */
  header = VALPTR_TO_HEADER0(ptr);
  length = HEADER_WORD_GET_SIZE(header);
  length -= HEADER_JSVALUES;
  length -= sizeof(FunctionFrame) >> LOG_BYTES_IN_JSVALUE;
  length -= HEADER_WORD_GET_EXTRA(header);
  for (i = 0; i < length; i++)
    trace_slot(ptr->locals + i);

  assert(ptr->locals[length - 1] == JS_UNDEFINED);  /* GC_DEBUG (cacary) */
}

STATIC void trace_StrCons(StrCons **ptrp)
{
  StrCons *ptr = *ptrp;

  if (test_and_mark_cell(ptr))
    return;

  /* trace_slot(&ptr->str); */ /* weak pointer */
  if (ptr->next != NULL)
    trace_StrCons(&ptr->next);
}

STATIC void trace_StrCons_ptr_array(StrCons ***ptrp, size_t length)
{
  StrCons **ptr = *ptrp;
  size_t i;
  if (test_and_mark_cell(ptr))
    return;

  for (i = 0; i < length; i++)
    if (ptr[i] != NULL)
      trace_StrCons(ptr + i);
}

#ifdef HIDDEN_CLASS
STATIC void trace_HiddenClass(HiddenClass **ptrp)
{
  HiddenClass *ptr = *ptrp;
  /* printf("Enter trace_HiddenClass\n"); */
  if (test_and_mark_cell(ptr))
    return;
  trace_HashTable(&hidden_map(ptr));
  /* printf("Exit trace_HiddenClass\n"); */
}
#endif

/*
 * we do not move context
 */
STATIC void trace_Context(Context **contextp)
{
  Context *context = *contextp;

  if (test_and_mark_cell(context))
    return;

  trace_slot(&context->global);
  trace_FunctionTable_array(&context->function_table, FUNCTION_TABLE_LIMIT);
  /* TODO: update spregs.cf which is an inner pointer to function_table */
  trace_FunctionFrame(&context->spreg.lp);
  trace_slot(&context->spreg.a);
  trace_slot(&context->spreg.err);

  trace_slot(&context->exhandler_stack);
  trace_slot(&context->lcall_stack);

  /* process stack */
  assert(!is_marked_cell(context->stack));
  mark_cell(context->stack);
  scan_stack(context->stack, context->spreg.sp, context->spreg.fp);
}

STATIC void trace_js_object(uintptr_t *ptrp)
{
  uintptr_t ptr = *ptrp;
  Object *obj = (Object *) ptr;

  assert(in_js_space((void *) ptr));
  if (is_marked_cell((void *) ptr))
    return;
  mark_cell((void *) ptr);

  /* common header */
#ifdef HIDDEN_CLASS
  trace_HiddenClass(&obj->class);
#else
  trace_HashTable(&obj->map);
#endif
  trace_JSValue_array(&obj->prop, obj->n_props);

  header_word_t hword;
  hword = HEADER_TYPE_GET_WORD(VALPTR_TO_HEADERPTR(ptr));
  switch (HEADER_WORD_GET_TYPE(hword)) {
  case HTAG_SIMPLE_OBJECT:
    break;
  case HTAG_ARRAY:
    {
      ArrayCell *a = (ArrayCell *) obj;
      size_t len = 0;
      if (a->length < a->size) {
        len = a->length;
      } else {
        len = a->size;
      }
      trace_JSValue_array(&a->body, len);
    }
    break;
  case HTAG_FUNCTION:
    /* TODO: func_table_entry holds an inner pointer */
    scan_FunctionTable(((FunctionCell *) obj)->func_table_entry);
    trace_FunctionFrame(&((FunctionCell *) obj)->environment);
    break;
  case HTAG_BUILTIN:
    break;
  case HTAG_ITERATOR:
    /* iterator does not have a common header */
    assert(0);
    break;
#ifdef USE_REGEXP
#ifdef need_normal_regexp
  case HTAG_REGEXP:
    trace_leaf_object((uintptr_t *)&((RegexpCell *)obj)->pattern);
    break;
#endif /* need_normal_regexp */
#endif /* USE_REGEXP */
  case HTAG_BOXED_STRING:
  case HTAG_BOXED_NUMBER:
  case HTAG_BOXED_BOOLEAN:
    trace_slot(&((BoxedCell *) obj)->value);
    break;
  default:
    assert(0);
  }
}

STATIC void trace_iterator(Iterator **ptrp)
{
  Iterator *obj = *ptrp;

  assert(in_js_space((void *) obj));
  if (is_marked_cell((void *) obj))
    return;
  mark_cell((void *) obj);
  if (obj->size > 0)
    trace_JSValue_array(&obj->body, obj->size);
}

STATIC void trace_JSValue_array(JSValue **ptrp, size_t length)
{
  JSValue *ptr = *ptrp;
  size_t i;

  if (in_js_space(ptr)) {
    if (test_and_mark_cell(ptr))
      return;
  }

  /* SCAN */
  for (i = 0; i < length; i++, ptr++)
    trace_slot(ptr);
}

STATIC void trace_slot(JSValue* ptr)
{
  JSValue jsv = *ptr;
  if (is_leaf_object(jsv)) {
    Tag tag = jsv & TAGMASK;
    jsv &= ~TAGMASK;
    trace_leaf_object((uintptr_t *) &jsv);
    *ptr = jsv | tag;
  } else if (is_iterator(jsv)) {
    /* iterator does not have common headers but does have pointers */
    Tag tag = jsv & TAGMASK;
    jsv &= ~TAGMASK;
    trace_iterator((Iterator **) &jsv);
    *ptr = jsv | tag;
  } else if (is_pointer(jsv)) {
    Tag tag = jsv & TAGMASK;
    jsv &= ~TAGMASK;
    trace_js_object((uintptr_t *) &jsv);
    *ptr = jsv | tag;
  }
}

STATIC void trace_root_pointer(void **ptrp)
{
  void *ptr = *ptrp;

  if ((((uintptr_t) ptr) & TAGMASK) != 0) {
    trace_slot((JSValue *) ptrp);
    return;
  }

  switch (obj_header_tag(ptr)) {
  case HTAG_PROP:
    printf("HTAG_PROP in trace_root_pointer\n"); break;
  case HTAG_ARRAY_DATA:
    printf("HTAG_ARRAY_DATA in trace_root_pointer\n"); break;
  case HTAG_FUNCTION_FRAME:
    trace_FunctionFrame((FunctionFrame **)ptrp); break;
  case HTAG_HASH_BODY:
    trace_HashTable((HashTable **)ptrp); break;
  case HTAG_STR_CONS:
    trace_StrCons((StrCons **)ptrp); break;
  case HTAG_CONTEXT:
    trace_Context((Context **)ptrp); break;
  case HTAG_STACK:
    printf("HTAG_STACK in trace_root_pointer\n"); break;
#ifdef HIDDEN_CLASS
  case HTAG_HIDDEN_CLASS:
    trace_HiddenClass((HiddenClass **)ptrp); break;
#endif
  default:
    trace_slot((JSValue *) ptrp);
    return;
  }
}

STATIC void scan_roots(Context *ctx)
{
  struct global_constant_objects *gconstsp = &gconsts;
  JSValue* p;
  int i;

  /*
   * global variables
   */

  for (p = (JSValue *) gconstsp; p < (JSValue *) (gconstsp + 1); p++) {
    trace_slot(p);
  }

  /*
   * global malloced objects
   * For simplicity, we do not use a `for' loop to visit every object
   * registered in the gobjects.
   */
#ifdef HIDDEN_CLASS
  trace_HiddenClass(&gobjects.g_hidden_class_0);
#endif

  /* function table: do not trace.
   *                 Used slots should be traced through Function objects
   */
  /* string table */
  trace_StrCons_ptr_array(&string_table.obvector, string_table.size);

  /*
   * Context
   */
  trace_Context(&ctx);

  /*
   * tmp root
   */
  /* old gc root stack */
  for (i = 0; i <= tmp_roots_sp; i++)
    trace_root_pointer((void **) tmp_roots[i]);
  /* new gc root stack */
  for (i = 0; i < gc_root_stack_ptr; i++)
    trace_root_pointer((void **) gc_root_stack[i]);
}

STATIC void scan_stack(JSValue* stack, int sp, int fp)
{
  while (1) {
    while (sp >= fp) {
      trace_slot(stack + sp);
      sp--;
    }
    if (sp < 0)
      return;
    fp = stack[sp--];                                     /* FP */
    trace_FunctionFrame((FunctionFrame **)(stack + sp));  /* LP */
    sp--;
    sp--;                                                 /* PC */
    scan_FunctionTable((FunctionTable *) stack[sp--]);    /* CF */
    /* TODO: fixup inner pointer (CF) */
  }
}


/*
 * Clear pointer field to StringCell whose mark bit is not set.
 * Unlink the StrCons from the string table.  These StrCons's
 * are collected in the next collection cycle.
 */
STATIC void weak_clear_StrTable(StrTable *table)
{
  size_t i;
  for (i = 0; i < table->size; i++) {
    StrCons ** p = table->obvector + i;
    while (*p != NULL) {
      StringCell *cell = remove_normal_string_tag((*p)->str);
      if (!is_marked_cell(cell)) {
        (*p)->str = JS_UNDEFINED;
        *p = (*p)->next;
      } else
        p = &(*p)->next;
    }
  }
}

STATIC void weak_clear(void)
{
  weak_clear_StrTable(&string_table);
}

STATIC void sweep_space(struct space *space)
{
  struct free_chunk **p;
  uintptr_t scan = space->addr;
  uintptr_t free_bytes = 0;

  GCLOG_SWEEP("sweep %s\n", space->name);

  space->freelist = NULL;
  p = &space->freelist;
  while (scan < space->addr + space->bytes) {
    uintptr_t last_used = 0;
    uintptr_t free_start;
    /* scan used area */
    while (scan < space->addr + space->bytes &&
           is_marked_cell_header((void *) scan)) {
      HeaderCell* header = (HeaderCell *) scan;
      size_t size = HEADER_GET_SIZE(header);
#ifdef GC_DEBUG
      assert(HEADER_GET_MAGIC(header) == HEADER_MAGIC);
#endif /* GC_DEBUG */
      unmark_cell_header((void *) scan);
      last_used = scan;
      scan += size << LOG_BYTES_IN_JSVALUE;
    }
    free_start = scan;
    while (scan < space->addr + space->bytes &&
           !is_marked_cell_header((void *) scan)) {
      HeaderCell* header = (HeaderCell *) scan;
      uint32_t size = HEADER_GET_SIZE(header);
#ifdef GC_DEBUG
      assert(HEADER_GET_MAGIC(header) == HEADER_MAGIC);
#endif /* GC_DEBUG */
      scan += size << LOG_BYTES_IN_JSVALUE;
    }
    if (free_start < scan) {
      if (last_used != 0) {
        HeaderCell* last_header = (HeaderCell *) last_used;
        uint32_t extra = HEADER_GET_EXTRA(last_header);
        uint32_t size = HEADER_GET_SIZE(last_header);
        free_start -= extra << LOG_BYTES_IN_JSVALUE;
        size -= extra;
        HEADER_SET_SIZE(last_header, size);
        HEADER_SET_EXTRA(last_header, 0);
      }
      if (scan - free_start >=
          MINIMUM_FREE_CHUNK_JSVALUES << LOG_BYTES_IN_JSVALUE) {
        struct free_chunk *chunk = (struct free_chunk *) free_start;
        GCLOG_SWEEP("add_cunk %x - %x (%d)\n",
                    free_start - space->addr, scan - space->addr,
                    scan - free_start);
#ifdef GC_DEBUG
        memset(chunk, 0xcc, scan - free_start);
#endif /* GC_DEBUG */
        chunk->header.header0 =
          HEADER0_COMPOSE((scan - free_start) >> LOG_BYTES_IN_JSVALUE,
                          0, HTAG_FREE);
#ifdef GC_DEBUG
        HEADER_SET_MAGIC(&(chunk->header), HEADER_MAGIC);
#endif /* GC_DEBUG */
        *p = chunk;
        p = &chunk->next;
        free_bytes += scan - free_start;
      } else  {
        ((HeaderCell *) free_start)->header0 =
          HEADER0_COMPOSE((scan - free_start) >> LOG_BYTES_IN_JSVALUE,
                          0, HTAG_FREE);
#ifdef GC_DEBUG
        HEADER_SET_MAGIC((HeaderCell *) free_start, HEADER_MAGIC);
#endif /* GC_DEBUG */
      }
    }
  }
  (*p) = NULL;
  space->free_bytes = free_bytes;
}


STATIC void sweep(void)
{
#ifdef GC_DEBUG
  sanity_check();
  check_invariant();
#endif /* GC_DEBUG */
  sweep_space(&js_space);
}

#ifdef GC_DEBUG
STATIC void check_invariant_nobw_space(struct space *space)
{
  uintptr_t scan = space->addr;

  while (scan < space->addr + space->bytes) {
    HeaderCell *hdrp = (HeaderCell *) scan;
    header_word_t header = HEADER_TYPE_GET_WORD(hdrp);
    if (HEADER_WORD_GET_TYPE(header) == HTAG_STRING)
      ;
#ifdef need_flonum
    else if (HEADER_WORD_GET_TYPE(header) == HTAG_FLONUM)
      ;
#endif
    else if (HEADER_WORD_GET_TYPE(header) == HTAG_CONTEXT)
      ;
    else if (HEADER_WORD_GET_TYPE(header) == HTAG_STACK)
      ;
    else if (is_marked_cell_header(hdrp)) {
      /* this object is black; should not contain a pointer to white */
      size_t payload_jsvalues = HEADER_GET_SIZE(hdrp);
      size_t i;
      payload_jsvalues -= HEADER_JSVALUES;
      payload_jsvalues -= HEADER_GET_EXTRA(hdrp);
      for (i = 0; i < payload_jsvalues; i++) {
        JSValue x = ((JSValue *) (scan + HEADER_BYTES))[i];
        if (HEADER_WORD_GET_TYPE(header) == HTAG_STR_CONS) {
          if (i ==
              (((uintptr_t) &((StrCons *) 0)->str) >> LOG_BYTES_IN_JSVALUE))
            continue;
        }
        if (is_pointer(x)) {
          if (in_js_space((void *)(x & ~TAGMASK))) {
            assert(is_marked_cell((void *) (x & ~TAGMASK)));
          }
        }
      }
    }
    scan += HEADER_GET_SIZE(hdrp) << LOG_BYTES_IN_JSVALUE;
  }
}

STATIC void check_invariant(void)
{
  check_invariant_nobw_space(&js_space);
}


STATIC void print_memory_status(void)
{
  GCLOG("  gc_disabled = %d\n", gc_disabled);
  GCLOG("  js_space.free_bytes = %d\n", js_space.free_bytes);
}

STATIC void print_heap_stat(void)
{
  size_t jsvalues[17] = {0, };
  size_t number[17] = {0, };
  uintptr_t scan = js_space.addr;
  size_t i;

  while (scan < js_space.addr + js_space.bytes) {
    HeaderCell* header = (HeaderCell *) scan;
    cell_type_t type = HEADER_GET_TYPE(header);
    size_t size = HEADER_GET_SIZE(header);
    if (type != HTAG_FREE) {
      jsvalues[type] += size;
      number[type] ++;
    }
    scan += (size << LOG_BYTES_IN_JSVALUE);
  }

  for (i = 0; i < 17; i++) {
    printf("type %02zu: num = %08zu volume = %08zu\n", i, number[i], jsvalues[i]);
  }
}

extern void** stack_start;
STATIC void sanity_check()
{
}
#endif /* GC_DEBUG */

#if defined GC_MARK_COMPACT
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

STATIC void compaction(Context *ctx)
{
  set_fwdptr();
  update_roots(ctx);
  move_heap_object();
}

STATIC void set_fwdptr(void)
{
  uintptr_t scan = js_space.addr;
  uintptr_t fwd = js_space.addr;

  while (scan < js_space.addr + js_space.bytes) {
    HeaderCell* header = (HeaderCell *) scan;
    cell_type_t type = HEADER_GET_TYPE(header);
    size_t size = HEADER_GET_SIZE(header);
    if (type != HTAG_FREE) {
      HEADER_SET_FWD(header, HEADERPTR_TO_VALPTR(fwd));
      fwd += (size << LOG_BYTES_IN_JSVALUE);
    }
    scan += (size << LOG_BYTES_IN_JSVALUE);
  }
}

/*
 * update address with fwdptr
 */

STATIC void update_JSValue(JSValue *jsvp);
STATIC void update_JSValue_array(JSValue *jsvarr, size_t len);
STATIC void update_ObjectCell(Object *obj);
STATIC void update_ArrayCell(ArrayCell *arr);
STATIC void update_FunctionCell(FunctionCell *func);
STATIC void update_FunctionFrame(FunctionFrame **pframe);
STATIC void update_Iterator(Iterator* itr);
STATIC void update_BoxedCell(BoxedCell *boxed);
STATIC void update_HiddenClass(HiddenClass **phc);
STATIC void update_HashTable(HashTable **pmap);
STATIC void update_HashBody(HashCell ***pbody, size_t len);
STATIC void update_HashCell(HashCell **phash);
STATIC void update_Context(Context *ctx);
STATIC void update_FunctionTable(FunctionTable *table);
STATIC void update_FunctionTable_array(FunctionTable *table, size_t len);
STATIC void update_StrCons(StrCons **ptrp);
STATIC void update_StrCons_ptr_array(StrCons ***ptrp, size_t length);
STATIC void update_root_ptr(void **ptrp);
STATIC void update_stack(JSValue** stack, int sp, int fp);


STATIC void update_roots(Context *ctx)
{
  int i;

  /*
   * global variables
   */
  {
    struct global_constant_objects *gconstsp = &gconsts;
    size_t len = ((uintptr_t)(gconstsp + 1) - (uintptr_t)gconstsp) / sizeof(JSValue);
    update_JSValue_array((JSValue *)gconstsp, len);
  }

  /*
   * global malloced objects
   * For simplicity, we do not use a `for' loop to visit every object
   * registered in the gobjects.
   */
#ifdef HIDDEN_CLASS
  update_HiddenClass(&(gobjects.g_hidden_class_0));
#endif

  /* function table: do not trace.
   *                 Used slots should be traced through Function objects
   */
  /* string table */
  update_StrCons_ptr_array(&string_table.obvector, string_table.size);

  /*
   * Context
   */
  update_Context(ctx);

  update_FunctionTable_array(&function_table[0], FUNCTION_TABLE_LIMIT);

  /*
   * tmp root
   */
  /* old gc root stack */
  for (i = 0; i <= tmp_roots_sp; i++)
    update_root_ptr((void **)tmp_roots[i]);

  /* new gc root stack */
  for (i = 0; i < gc_root_stack_ptr; i++)
    update_root_ptr((void **)gc_root_stack[i]);
}

STATIC void update_regbase(size_t diff)
{
  int i;
  for (i = 0; i < gc_regbase_stack_ptr; i++) {
    JSValue **pregbase = gc_regbase_stack[i];
    uintptr_t regbase = (uintptr_t)*pregbase;
    *pregbase = (JSValue *)(regbase - diff);
  }
}

STATIC void move_heap_object(void)
{
  uintptr_t scan = js_space.addr;
  size_t used = 0;
  struct free_chunk* tail = (struct free_chunk *)scan;

  while (scan < js_space.addr + js_space.bytes) {
    HeaderCell* header = (HeaderCell *) scan;
    cell_type_t type = HEADER_GET_TYPE(header);
    size_t size = HEADER_GET_SIZE(header);

    if (type != HTAG_FREE) {
      JSValue* fwd = (JSValue *)VALPTR_TO_HEADERPTR(HEADER_GET_FWD(header));
      HEADER_SET_FWD(header, 0);
      HEADER_SET_GC(header, 0);
#ifdef GC_DEBUG
      {
        HeaderCell* shadow = get_shadow(fwd);
        *shadow = *header;
      }
#endif /* GC_DEBUG */
      if ((JSValue *)header != fwd) {
        JSValue* p = (JSValue *)header;
        JSValue* end = (JSValue *)header + size;
        for(; p < end; ++p, ++fwd) {
          *fwd = *p;
        }
      }

      tail = (struct free_chunk *)fwd;
      used += size;
    }
    scan += (size << LOG_BYTES_IN_JSVALUE);
  }

  size_t freesize = (js_space.bytes >> LOG_BYTES_IN_JSVALUE) - used;
  tail->header.header0 = HEADER0_COMPOSE(freesize, 0, HTAG_FREE);
#ifdef GC_DEBUG
  HEADER_SET_MAGIC(&(tail->header), HEADER_MAGIC);
#endif
  tail->header.header1 = 0;
  tail->next = NULL;
  js_space.freelist = tail;
}


/*
 * Tracer
 */

STATIC void update_JSValue(JSValue *jsvp)
{
  JSValue jsv = *jsvp;
  if (!is_pointer(jsv))
    return;

  void *ptr = (void *)clear_tag(jsv);
  Tag tag = get_tag(jsv);
  if (!in_js_space(ptr))
    return;

  HeaderCell* hdrp = VALPTR_TO_HEADERPTR(ptr);
  *jsvp = put_tag(HEADER_GET_FWD(hdrp), tag);

  switch(HEADER_GET_TYPE(hdrp)) {
    case HTAG_FREE:
      LOG_EXIT("update_JSValue : htag is HTAG_FREE\n");
      return;

    case HTAG_STRING:
    case HTAG_FLONUM:
      // Nothing to do
      break;

    case HTAG_SIMPLE_OBJECT:
      update_ObjectCell((Object *)ptr);
      break;
    case HTAG_ARRAY:
      update_ArrayCell((ArrayCell *)ptr);
      break;
    case HTAG_FUNCTION:
      update_FunctionCell((FunctionCell *)ptr);
      break;
    case HTAG_BUILTIN:
      // Same as ObjectCell
      update_ObjectCell((Object *)ptr);
      break;
    case HTAG_ITERATOR:
      update_Iterator((Iterator *)ptr);
      break;
#ifdef use_regexp
    case HTAG_REGEXP:
      LOG_EXIT("Not implemented\n");
      return;
#endif
    case HTAG_BOXED_STRING:
    case HTAG_BOXED_NUMBER:
    case HTAG_BOXED_BOOLEAN:
      update_BoxedCell((BoxedCell *)ptr);
      break;


    case HTAG_PROP:
    case HTAG_ARRAY_DATA:
    case HTAG_FUNCTION_FRAME:
    case HTAG_HASH_BODY:
    case HTAG_STR_CONS:
    case HTAG_CONTEXT:
    case HTAG_STACK:
    case HTAG_HIDDEN_CLASS:
      LOG_EXIT("Unreachable Code\n");
      abort();
      break;
    
    default:
     LOG_EXIT("Unknown tag : 0x%04"PRIJSValue"\n", HEADER_GET_TYPE(hdrp));
     abort();
  }
}

STATIC void update_JSValue_array(JSValue *jsvarr, size_t len)
{
  size_t i;
  for (i = 0; i < len; ++i) {
    update_JSValue(jsvarr + i);
  }
}

STATIC void update_ObjectCell(Object *obj)
{
  assert(in_js_space(obj));
  if (is_marked_cell(obj)) return;

  mark_cell(obj);

  update_HiddenClass(&(obj->class));

  JSValue **prop = &(obj->prop);
  assert(in_js_space(*prop));
  if (!is_marked_cell(*prop)) {
    update_JSValue_array(*prop, obj->limit_props);
    mark_cell(*prop);
  }
  *prop = (JSValue *)HEADER_GET_FWD(VALPTR_TO_HEADERPTR(*prop));
}

STATIC void update_ArrayCell(ArrayCell *arr)
{
  assert(in_js_space(arr));
  if (is_marked_cell(arr)) return;

  // Note: marked in update_ObjectCell
  // mark_cell(arr);
  
  update_ObjectCell(&(arr->o));

  JSValue **body = &(arr->body);
  if (*body != NULL) {
    assert(in_js_space(*body));
    if (!is_marked_cell(*body)) {
      size_t len = 0;
      if (arr->length < arr->size) {
        len = arr->length;
      } else {
        len = arr->size;
      }
      update_JSValue_array(*body, len);
      mark_cell(*body);
    }
    *body = (JSValue *)HEADER_GET_FWD(VALPTR_TO_HEADERPTR(*body));
  }
}

STATIC void update_FunctionCell(FunctionCell *func)
{
  assert(in_js_space(func));
  if (is_marked_cell(func)) return;

  // Note: marked in update_ObjectCell
  // mark_cell(func);

  update_ObjectCell(&(func->o));

  // All FunctionTable objects are updated
  // by calling update_FunctionTable_array
  // at update_roots.
  // Nothing to do
  assert(!in_js_space(func->func_table_entry));

  update_FunctionFrame(&(func->environment));
}

STATIC void update_FunctionFrame(FunctionFrame **pframe)
{
  FunctionFrame *frame = *pframe;
  HeaderCell *hdrp = VALPTR_TO_HEADERPTR(frame);
  assert(in_js_space(frame));
  if (is_marked_cell(frame)) {
    *pframe = (FunctionFrame *)HEADER_GET_FWD(hdrp);
    return;
  }
  mark_cell(frame);

  FunctionFrame **prev = &(frame->prev_frame);
  if (*prev != NULL) {
    update_FunctionFrame(prev);
  }

  header_word_t size = HEADER_GET_SIZE(hdrp);
  JSValue *vals = &(frame->arguments);
  JSValue *end = ((JSValue *)hdrp) + size;
  size_t len = ((uintptr_t)end - (uintptr_t)vals) / sizeof(JSValue);
  update_JSValue_array(vals, len);

  *pframe = (FunctionFrame *)HEADER_GET_FWD(hdrp);
}

STATIC void update_Iterator(Iterator *itr)
{
  assert(in_js_space(itr));
  if (is_marked_cell(itr)) return;

  mark_cell(itr);

  JSValue **body = &(itr->body);
  if (*body != NULL) {
    assert(in_js_space(*body));
    if (!is_marked_cell(*body)) {
      update_JSValue_array(*body, itr->size);
      mark_cell(*body);
    }
    *body = (JSValue *)HEADER_GET_FWD(VALPTR_TO_HEADERPTR(*body));
  }
}

STATIC void update_BoxedCell(BoxedCell *boxed)
{
  assert(in_js_space(boxed));
  if (is_marked_cell(boxed)) return;

  // Note: marked in update_ObjectCell
  // mark_cell(boxed);

  update_ObjectCell(&(boxed->o));

  update_JSValue(&(boxed->value));
}

STATIC void update_HiddenClass(HiddenClass **phc)
{
  HiddenClass *hc = *phc;
  assert(in_js_space(hc));
  if (is_marked_cell(hc)) {
    *phc = (HiddenClass *)HEADER_GET_FWD(VALPTR_TO_HEADERPTR(hc));
    return;
  }
  mark_cell(hc);

  update_HashTable(&hidden_map(hc));

  *phc = (HiddenClass *)HEADER_GET_FWD(VALPTR_TO_HEADERPTR(hc));
}

STATIC void update_HashTable(HashTable **pmap)
{
  HashTable *map = *pmap;
  assert(!in_js_space(map));
  
  update_HashBody(&(map->body), map->size);
}

STATIC void update_HashBody(HashCell ***pbody, size_t len)
{
  HashCell **body = *pbody;
  assert(in_js_space(body));
  if (is_marked_cell(body)) {
    *pbody = (HashCell **)HEADER_GET_FWD(VALPTR_TO_HEADERPTR(body));
    return;
  }
  mark_cell(body);

  size_t i;
  for (i = 0; i < len; ++i) {
    if (body[i] != NULL) update_HashCell(&body[i]);
  }

  *pbody = (HashCell **)HEADER_GET_FWD(VALPTR_TO_HEADERPTR(body));
}

STATIC void update_HashCell(HashCell **phash)
{
  HashCell *hash = *phash;
  assert(!in_js_space(hash));

  update_JSValue((JSValue *)&(hash->entry.key));

  if (is_transition(hash->entry.attr)) {
    update_HiddenClass((HiddenClass **)&(hash->entry.data));
  }

  if (hash->next != NULL) {
    update_HashCell(&(hash->next));
  }
}

STATIC void update_StrCons(StrCons **pcons)
{
  StrCons *cons = *pcons;
  assert(in_js_space(cons));
  if (is_marked_cell(cons)) {
    *pcons = (StrCons *)HEADER_GET_FWD(VALPTR_TO_HEADERPTR(cons));
    return;
  }
  mark_cell(cons);

  update_JSValue(&(cons->str));

  if (cons->next != NULL) {
    update_StrCons(&(cons->next));
  }

  *pcons = (StrCons *)HEADER_GET_FWD(VALPTR_TO_HEADERPTR(cons));
}

STATIC void update_StrCons_ptr_array(StrCons ***ptrp, size_t length)
{
  StrCons **ptr = *ptrp;
  assert(!in_js_space(ptr));
  size_t i;

  for (i = 0; i < length; i++)
    if (ptr[i] != NULL) {
      update_StrCons(ptr + i);
    }
}

STATIC void update_Context(Context *ctx)
{
    update_JSValue(&(ctx->global));

    // Nothing to do
    assert(!in_js_space(ctx->function_table));

    update_FunctionFrame(&(ctx->spreg.lp));

    update_JSValue(&(ctx->spreg.a));
    update_JSValue(&(ctx->spreg.err));

    update_JSValue(&(ctx->exhandler_stack));
    update_JSValue(&(ctx->lcall_stack));

    update_stack(&(ctx->stack), ctx->spreg.sp, ctx->spreg.fp);
}

STATIC void update_stack(JSValue** pstack, int sp, int fp)
{
  JSValue *stack = *pstack;
  assert(in_js_space(stack));
  if (is_marked_cell(stack)) {
    *pstack = (JSValue *)HEADER_GET_FWD(VALPTR_TO_HEADERPTR(stack));
    return;
  }
  mark_cell(stack);

  while (1) {
    while (sp >= fp) {
      update_JSValue(stack + sp);
      sp--;
    }
    if (sp < 0)
      break;

    fp = stack[sp--];                                        /* FP */
    update_FunctionFrame((FunctionFrame **) &(stack[sp--])); /* LP */
    sp--;                                                    /* PC */
    assert(!in_js_space((void *) stack[sp]));
    sp--;                                                    /* CF */
    /* TODO: fixup inner pointer (CF) */
  }

  uintptr_t stack_old = (uintptr_t)stack;
  *pstack = (JSValue *)HEADER_GET_FWD(VALPTR_TO_HEADERPTR(*pstack));

  uintptr_t stack_new = (uintptr_t)*pstack;
  assert(stack_old >= stack_new);
  if (stack_old > stack_new) {
    update_regbase(stack_old - stack_new);
  }
}

STATIC void update_FunctionTable(FunctionTable *table)
{
  assert(!in_js_space(table));

  Instruction *insns = table->insns;
  assert(!in_js_space(insns));

  JSValue *consts = (JSValue *)(insns + table->n_insns);
  update_JSValue_array(consts, table->n_constants);
}

STATIC void update_FunctionTable_array(FunctionTable *tablearr, size_t len)
{
  size_t i;
  for (i = 0; i < len; ++i) {
    update_FunctionTable(tablearr + i);
  }
}

STATIC void update_root_ptr(void **ptrp)
{
  void *ptr = *ptrp;

  if (clear_tag(ptr) != 0) {
    JSValue *pjsv = (JSValue *)ptrp;
    update_JSValue(pjsv);
    return;
  }

  switch (obj_header_tag(ptr)) {
  case HTAG_PROP:
    printf("HTAG_PROP in update_root_ptr\n"); break;
  case HTAG_ARRAY_DATA:
    printf("HTAG_ARRAY_DATA in update_root_ptr\n"); break;
  case HTAG_FUNCTION_FRAME:
    update_FunctionFrame((FunctionFrame **)ptrp);
    break;
  case HTAG_HASH_BODY:
    {
      HeaderCell *hdrp = VALPTR_TO_HEADERPTR(ptr);
      header_word_t size = HEADER_GET_SIZE(hdrp);
      uintptr_t end = (uintptr_t)((JSValue *)hdrp + size);
      size_t diff = (size_t)(end - (uintptr_t)ptr) / sizeof(HashCell *);
      update_HashBody((HashCell ***)ptrp, diff);
    }
    break;
  case HTAG_STR_CONS:
    update_StrCons((StrCons **)ptrp);
    break;
  case HTAG_CONTEXT:
    printf("HTAG_CONTEXT in update_root_ptr\n");
    break;
  case HTAG_STACK:
    printf("HTAG_STACK in update_root_ptr\n");
    break;
#ifdef HIDDEN_CLASS
  case HTAG_HIDDEN_CLASS:
    update_HiddenClass((HiddenClass **)ptrp);
    break;
#endif
  default:
    LOG_EXIT("GC::UPDATE_ROOT_PTR : unknown data type");
//    update_JSValue((JSValue *) ptrp);
    return;
  }
}
#endif /* defined GC_MARK_COMPACT */
#endif /* (defined GC_MARK_SWEEP) || (defined GC_MARK_COMPACT) */


#ifdef GC_CLEAR_MEM
STATIC void fill_free_cell(struct free_chunk *p, JSValue val)
{
  while(p != NULL) {
    struct free_chunk *chunk = p;
    p = p->next;

    size_t size = HEADER_GET_SIZE(&(chunk->header));
    JSValue *head = (JSValue *)(chunk + 1);
    JSValue *end = ((JSValue *)chunk) + size;
    while(head < end) {
      *head = val;
      ++head;
    }
  }
}
#endif

/* Local Variables:      */
/* mode: c               */
/* c-basic-offset: 2     */
/* indent-tabs-mode: nil */
/* End:                  */
