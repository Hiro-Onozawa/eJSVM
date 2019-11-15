/*
 * eJS Project
 * Kochi University of Technology
 * The University of Electro-communications
 *
 * The eJS Project is the successor of the SSJS Project at The University of
 * Electro-communications.
 */

#include "gc_mark_common.c"

/*
 * prototype
 */
STATIC void threaded_compaction(Context *ctx);
STATIC void update_forward_reference(Context *ctx);
STATIC void update_backward_reference();
STATIC void thread_reference(void **ref);
STATIC void update_reference(void *ref, void *addr);
STATIC header_word_t get_threaded_header(HeaderCell *hdrp);

/*
 *  Space
 */
STATIC void create_space(struct space *space, size_t bytes, char *name)
{
  struct free_chunk *p;
  p = (struct free_chunk *) malloc(bytes);
  HEADER_COMPOSE(&(p->header), bytes >> LOG_BYTES_IN_JSVALUE, HTAG_FREE);
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
      /* This chunk is large enough to leave a part unused.  Split it */
      size_t new_chunk_jsvalues = chunk_jsvalues - alloc_jsvalues;
      uintptr_t addr =
        ((uintptr_t) chunk) + (new_chunk_jsvalues << LOG_BYTES_IN_JSVALUE);
      HEADER_SET_SIZE(&chunk->header, new_chunk_jsvalues);
      HEADER_COMPOSE((HeaderCell *) addr, alloc_jsvalues, type);
#ifdef GC_DEBUG
      HEADER_SET_MAGIC((HeaderCell *) addr, HEADER_MAGIC);
      HEADER_SET_GEN_MASK((HeaderCell *) addr, generation);
#endif /* GC_DEBUG */
      space->free_bytes -= alloc_jsvalues << LOG_BYTES_IN_JSVALUE;
      return HEADERPTR_TO_VALPTR(addr);
    }
  }

  printf("memory exhausted\n");
  return NULL;
}


/*
 * GC
 */
STATIC void collect(Context *ctx) {
  scan_roots(ctx);
  weak_clear();
  threaded_compaction(ctx);
}

STATIC void thread_roots(Context* ctx);
STATIC void thread_JSValue(JSValue *jsvp);
STATIC void thread_JSValue_array(JSValue *jsvarr, size_t len);
STATIC void thread_ObjectCell(Object *obj);
STATIC void thread_ArrayCell(ArrayCell *arr);
STATIC void thread_FunctionCell(FunctionCell *func);
STATIC void thread_FunctionTable(FunctionTable *table);
STATIC void thread_FunctionTable_array(FunctionTable *table, size_t len);
STATIC void thread_Iterator(Iterator* itr);
STATIC void thread_BoxedCell(BoxedCell *boxed);
STATIC void thread_FunctionFrame(FunctionFrame *pframe);
STATIC void thread_HashBody(HashCell **pbody);
STATIC void thread_HashCell(HashCell **phash);
STATIC void thread_StrCons(StrCons *ptr);
STATIC void thread_StrCons_ptr_array(StrCons ***pptrarr, size_t length);
STATIC void thread_HiddenClass(HiddenClass *phc);
STATIC void thread_Context(Context *ctx);
STATIC void thread_stack(JSValue** stack, int sp, int fp);
STATIC void thread_root_ptr(void **ptrp);
STATIC void update_regbase(intptr_t diff);

STATIC void thread_fields(void *pval);
STATIC int is_reference(void **pptr);

STATIC header_word_t get_threaded_header(HeaderCell *hdrp)
{
  void **pref = (void **) hdrp->header0;
  while (is_reference(pref)) {
    pref = (void **) (*pref);
  }
  return (header_word_t) pref;
}

STATIC void threaded_compaction(Context *ctx)
{
  update_forward_reference(ctx);
  update_backward_reference();
}

STATIC void thread_roots(Context *ctx)
{
  struct global_constant_objects *gconstsp = &gconsts;
  JSValue* p;
  int i;

  /*
   * global variables
   */

  for (p = (JSValue *) gconstsp; p < (JSValue *) (gconstsp + 1); p++) {
    thread_JSValue(p);
  }

  /*
   * global malloced objects
   * For simplicity, we do not use a `for' loop to visit every object
   * registered in the gobjects.
   */
#ifdef HIDDEN_CLASS
  thread_reference((void **) &gobjects.g_hidden_class_0);
#endif

  /* function table */
  thread_FunctionTable_array(&function_table[0], FUNCTION_TABLE_LIMIT);

  /* string table */
  thread_StrCons_ptr_array(&string_table.obvector, string_table.size);

  /*
   * Context
   */
  thread_Context(ctx);

  /*
   * tmp root
   */
  /* old gc root stack */
  for (i = 0; i <= tmp_roots_sp; i++)
    thread_root_ptr((void **) tmp_roots[i]);
  /* new gc root stack */
  for (i = 0; i < gc_root_stack_ptr; i++)
    thread_root_ptr((void **) gc_root_stack[i]);
}

STATIC void thread_JSValue(JSValue *jsvp)
{
  JSValue jsv = *jsvp;

  if (!is_pointer(jsv))
    return;

  *jsvp = (JSValue) clear_tag(jsv);
  thread_reference((void **) jsvp);
}

STATIC void thread_JSValue_array(JSValue *jsvarr, size_t len)
{
  size_t i;
  for (i = 0; i < len; ++i) {
    thread_JSValue(jsvarr + i);
  }
}

STATIC void thread_ObjectCell(Object *obj)
{
  thread_reference((void **) &(obj->class));
  thread_reference((void **) &(obj->prop));
}

STATIC void thread_ArrayCell(ArrayCell *arr)
{
  thread_ObjectCell(&(arr->o));

  thread_reference((void **) &(arr->body));
}

STATIC void thread_FunctionCell(FunctionCell *func)
{
  thread_ObjectCell(&(func->o));

  assert(!in_js_space(func->func_table_entry));
  thread_reference((void **) &(func->environment));
}

STATIC void thread_FunctionTable(FunctionTable *table)
{
  JSValue *head = (JSValue *) &(table->insns[table->n_insns]);
  thread_JSValue_array(head, table->n_constants);
}

STATIC void thread_FunctionTable_array(FunctionTable *table, size_t len)
{
  size_t i;
  for (i = 0; i < len; ++i) {
    thread_FunctionTable(&(table[i]));
  }
}

STATIC void thread_Iterator(Iterator* itr)
{
  thread_reference((void **) &(itr->body));
}

STATIC void thread_BoxedCell(BoxedCell *boxed)
{
  thread_ObjectCell(&(boxed->o));

  thread_JSValue(&(boxed->value));
}

STATIC void thread_FunctionFrame(FunctionFrame *pframe)
{
  HeaderCell *hdrp;
  header_word_t header0;
  header_word_t size;
  JSValue *begin;
  JSValue *end;
  size_t len;

  thread_reference((void **) &(pframe->prev_frame));

  thread_JSValue(&(pframe->arguments));

  hdrp = VALPTR_TO_HEADERPTR(pframe);
  header0 = get_threaded_header(hdrp);
  size = HEADERW_GET_SIZE(header0);
  begin = pframe->locals;
  end = ((JSValue *) hdrp) + size;
  len = (((uintptr_t) end) - ((uintptr_t) begin)) / sizeof(JSValue);
  thread_JSValue_array(begin, len);
}

STATIC void thread_HashBody(HashCell **pbody)
{
  HeaderCell *hdrp;
  header_word_t header0;
  header_word_t size;
  header_word_t i;

  hdrp = VALPTR_TO_HEADERPTR(pbody);
  header0 = get_threaded_header(hdrp);
  size = HEADERW_GET_SIZE(header0) - HEADER_JSVALUES;

  for (i = 0; i < size; ++i) {
    if (pbody[i] != NULL) thread_HashCell(&(pbody[i]));
  }
}

STATIC void thread_HashCell(HashCell **phash)
{
  HashCell *hash = *phash;
  assert(!in_js_space(hash));

  thread_JSValue((JSValue *)&(hash->entry.key));

  if (is_transition(hash->entry.attr)) {
    thread_reference((void **) ((HiddenClass **) &(hash->entry.data)));
  }

  if (hash->next != NULL) {
    thread_HashCell(&(hash->next));
  }
}

STATIC void thread_StrCons(StrCons *ptr)
{
  thread_JSValue(&(ptr->str));
  thread_reference((void **) &(ptr->next));
}

STATIC void thread_StrCons_ptr_array(StrCons ***pptrarr, size_t length)
{
  assert(!in_js_space(pptrarr));

  StrCons **ptrarr = *pptrarr;
  size_t i;

  for (i = 0; i < length; ++i) {
    if (ptrarr[i] != NULL) thread_reference((void **) &(ptrarr[i]));
  }
}

STATIC void thread_HiddenClass(HiddenClass *phc)
{
  HashTable *map;

  map = phc->map;
  assert(!in_js_space(map));

  thread_reference((void **) &(map->body));
}

STATIC void thread_Context(Context *ctx)
{
  thread_JSValue(&(ctx->global));

  // Nothing to do
  assert(!in_js_space(ctx->function_table));

  thread_reference((void **) &(ctx->spreg.lp));

  thread_JSValue(&(ctx->spreg.a));
  thread_JSValue(&(ctx->spreg.err));

  thread_JSValue(&(ctx->exhandler_stack));
  thread_JSValue(&(ctx->lcall_stack));

  thread_stack(&(ctx->stack), ctx->spreg.sp, ctx->spreg.fp);
}

STATIC void thread_stack(JSValue** pstack, int sp, int fp)
{
  JSValue *stack = *pstack;
  assert(in_js_space(stack));
  thread_reference((void **) pstack);

  while (1) {
    while (sp >= fp) {
      thread_JSValue(stack + sp);
      sp--;
    }
    if (sp < 0)
      break;

    fp = stack[sp--];                                    /* FP */
    thread_reference((void **) ((FunctionFrame **) &(stack[sp--]))); /* LP */
    sp--;                                                /* PC */
    assert(!in_js_space((void *) stack[sp]));
    sp--;                                                /* CF */
    /* TODO: fixup inner pointer (CF) */
  }
}

STATIC void thread_root_ptr(void **ptrp)
{
  void *ptr = *ptrp;

  if (get_tag(ptr) != 0) {
    JSValue *pjsv = (JSValue *) ptrp;
    thread_JSValue(pjsv);
    return;
  }

  switch (obj_header_tag(ptr)) {
  case HTAG_PROP:
    printf("HTAG_PROP in thread_root_ptr\n");
    break;
  case HTAG_ARRAY_DATA:
    printf("HTAG_ARRAY_DATA in thread_root_ptr\n");
    break;
  case HTAG_FUNCTION_FRAME:
    thread_reference((void **) ((FunctionFrame **) ptrp));
    break;
  case HTAG_HASH_BODY:
    thread_reference((void **) ((HashCell ***) ptrp));
    break;
  case HTAG_STR_CONS:
    thread_reference((void **) ((StrCons **) ptrp));
    break;
  case HTAG_CONTEXT:
    printf("HTAG_CONTEXT in thread_root_ptr\n");
    break;
  case HTAG_STACK:
    printf("HTAG_STACK in thread_root_ptr\n");
    break;
#ifdef HIDDEN_CLASS
  case HTAG_HIDDEN_CLASS:
    thread_reference((void **) ((HiddenClass **) ptrp));
    break;
#endif
  default:
    thread_JSValue((JSValue *) ptrp);
    return;
  }
}

STATIC void thread_fields(void *pval)
{
  HeaderCell *hdrp = VALPTR_TO_HEADERPTR(pval);
  header_word_t header0 = get_threaded_header(hdrp);
  switch(HEADERW_GET_TYPE(header0)) {
    case HTAG_FREE:
      LOG_EXIT("thread_fields : htag is HTAG_FREE\n");
      return;

    case HTAG_STRING:
    case HTAG_FLONUM:
      // Nothing To Do
      break;

    case HTAG_SIMPLE_OBJECT:
      thread_ObjectCell((Object *) pval);
      break;
    case HTAG_ARRAY:
      thread_ArrayCell((ArrayCell *) pval);
      break;
    case HTAG_FUNCTION:
      thread_FunctionCell((FunctionCell *) pval);
      break;
    case HTAG_BUILTIN:
      // Same as ObjectCell
      thread_ObjectCell((Object *) pval);
      break;
    case HTAG_ITERATOR:
      thread_Iterator((Iterator *) pval);
      break;
#ifdef use_regexp
    case HTAG_REGEXP:
      LOG_EXIT("Not implemented\n");
      return;
#endif
    case HTAG_BOXED_STRING:
    case HTAG_BOXED_NUMBER:
    case HTAG_BOXED_BOOLEAN:
      thread_BoxedCell((BoxedCell *) pval);
      break;


    case HTAG_PROP:
    case HTAG_ARRAY_DATA:
    {
      size_t size = HEADERW_GET_SIZE(header0) - HEADER_JSVALUES;
      thread_JSValue_array((JSValue *) pval, size);
      break;
    }
    case HTAG_FUNCTION_FRAME:
      thread_FunctionFrame((FunctionFrame *) pval);
      break;
    case HTAG_HASH_BODY:
      thread_HashBody((HashCell **) pval);
      break;
    case HTAG_STR_CONS:
      thread_StrCons((StrCons *) pval);
      break;
    case HTAG_CONTEXT:
      LOG_EXIT("Unreachable Code\n");
      abort();
      break;
    case HTAG_STACK:
      // Threaded at thread_roots
      break;
    case HTAG_HIDDEN_CLASS:
      thread_HiddenClass((HiddenClass *) pval);
      break;
    
    default:
     LOG_EXIT("Unknown tag : 0x%04"PRIJSValue"\n", HEADER_GET_TYPE(hdrp));
     abort();
  }
}

STATIC void update_forward_reference(Context *ctx)
{
  thread_roots(ctx);

  uintptr_t scan, free;
  scan = js_space.addr;
  free = js_space.addr;

  while (scan < js_space.addr + js_space.bytes) {
    HeaderCell *hdrp = (HeaderCell *) scan;
    header_word_t header0 = get_threaded_header(hdrp);
    header_word_t size = HEADERW_GET_SIZE(header0);

    if (HEADERW_GET_GC(header0)) {
      void *pval = HEADERPTR_TO_VALPTR(hdrp);
      update_reference(pval, HEADERPTR_TO_VALPTR(free));
      thread_fields(pval);
      free += size << LOG_BYTES_IN_JSVALUE;
    }

    scan += size << LOG_BYTES_IN_JSVALUE;
  }
}

STATIC void update_backward_reference()
{
  uintptr_t scan, free;
  scan = js_space.addr;
  free = js_space.addr;

  while (scan < js_space.addr + js_space.bytes) {
    HeaderCell *hdrp = (HeaderCell *) scan;
    header_word_t header0 = get_threaded_header(hdrp);
    header_word_t size = HEADERW_GET_SIZE(header0);

    if (HEADERW_GET_GC(header0)) {
      void *pval = HEADERPTR_TO_VALPTR(hdrp);
      update_reference(pval, HEADERPTR_TO_VALPTR(free));

      if (HEADERW_GET_TYPE(header0) == HTAG_STACK) {
        intptr_t stack_old = (intptr_t)scan;
        intptr_t stack_new = (intptr_t)free;
        update_regbase(stack_old - stack_new);
      }

      header_word_t type = HEADERW_GET_TYPE(header0);
      HEADER_COMPOSE(hdrp, size, type);
#ifdef GC_DEBUG
      HEADER_SET_MAGIC(hdrp, HEADER_MAGIC);
      
      {
        HeaderCell* shadow = get_shadow(free);
        *shadow = *hdrp;
      }
#endif /* GC_DEBUG */

      JSValue *from = (JSValue *)scan;
      JSValue *end = from + size;
      JSValue *to = (JSValue *)free;
      while (from < end) {
        *to = *from;
        ++to;
        ++from;
      }

      free += size << LOG_BYTES_IN_JSVALUE;
    }

    scan += size << LOG_BYTES_IN_JSVALUE;
  }

  size_t freebytes = (js_space.addr + js_space.bytes) - free;
  struct free_chunk *cell = (struct free_chunk *) free;
  HEADER_COMPOSE(&(cell->header), freebytes >> LOG_BYTES_IN_JSVALUE, HTAG_FREE);
#ifdef GC_DEBUG
  HEADER_SET_MAGIC(&(cell->header), HEADER_MAGIC);
#endif /* GC_DEBUG */
  cell->next = NULL;
  js_space.freelist = cell;
  js_space.free_bytes = freebytes;
}

STATIC void thread_reference(void **ref)
{
  if (HEADERW_GET_MARK((header_word_t) ref) != 0) {
    fprintf(stderr, "refernece %p is unthreadable address.\n", ref);
    abort();
    return;
  }

  if (*ref != NULL) {
    if (in_js_space(*ref)) {
      HeaderCell *cell = VALPTR_TO_HEADERPTR(*ref);
      header_word_t header0 = cell->header0;
      cell->header0 = (header_word_t) ref;
      *ref = (void *) header0;
    }
  }
}

STATIC int is_reference(void **pptr)
{
  header_word_t header0 = (header_word_t) (pptr);

  return HEADERW_GET_MARK(header0) == 0;
}

STATIC void update_regbase(intptr_t diff)
{
  int i;
  for (i = 0; i < gc_regbase_stack_ptr; i++) {
    JSValue **pregbase = gc_regbase_stack[i];
    intptr_t regbase = (intptr_t)*pregbase;
    *pregbase = (JSValue *)(regbase - diff);
  }
}

STATIC void update_reference(void *ref, void *addr)
{
  HeaderCell *cell = VALPTR_TO_HEADERPTR(ref);
  header_word_t header0 = get_threaded_header(cell);
  header_word_t type = HEADERW_GET_TYPE(header0);
  Tag tag = 0;

  switch(type) {
    case HTAG_FREE:
      LOG_EXIT("unreachable code.");
      abort();
      break;

    case HTAG_STRING:
      tag = T_STRING;
      break;
    case HTAG_FLONUM:
#ifdef BIT_32
      tag = T_GENERIC;
#else
      tag = T_FLONUM;
#endif
      break;
    case HTAG_SIMPLE_OBJECT:
      tag = T_GENERIC;
      break;
    case HTAG_ARRAY:
      tag = T_GENERIC;
      break;
    case HTAG_FUNCTION:
      tag = T_GENERIC;
      break;
    case HTAG_BUILTIN:
      tag = T_GENERIC;
      break;
    case HTAG_ITERATOR:
      tag = T_GENERIC;
      break;
#ifdef use_regexp
    case HTAG_REGEXP:
      LOG_EXIT("Not Implemented");
      break;
#endif
    case HTAG_BOXED_STRING:
    case HTAG_BOXED_NUMBER:
    case HTAG_BOXED_BOOLEAN:
      tag = T_GENERIC;
      break;


    case HTAG_PROP:
    case HTAG_ARRAY_DATA:
    case HTAG_FUNCTION_FRAME:
    case HTAG_HASH_BODY:
    case HTAG_STR_CONS:
    case HTAG_CONTEXT:
    case HTAG_STACK:
    case HTAG_HIDDEN_CLASS:
      tag = (Tag)(-1);
      break;
      
    default:
      LOG_EXIT("Unreachable code.");
      abort();
      break;
  }

  void **tmp = (void **) cell->header0;
  while(is_reference(tmp)) {
    void **next = (void **) *tmp;
    if (tag != (Tag)(-1)) {
      *tmp = (void *) put_tag(addr, tag);
    }
    else {
      *tmp = addr;
    }
    tmp = next;
  }
  cell->header0 = (header_word_t) tmp;
}

/* Local Variables:      */
/* mode: c               */
/* c-basic-offset: 2     */
/* indent-tabs-mode: nil */
/* End:                  */
