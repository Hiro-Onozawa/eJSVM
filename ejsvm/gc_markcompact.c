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
STATIC void compaction(Context *ctx);
STATIC void set_fwdptr(void);
STATIC void update_roots(Context *ctx);
STATIC void update_regbase(size_t diff);
STATIC void move_heap_object(void);

/*
 *  Space
 */
STATIC void create_space(struct space *space, size_t bytes, char *name)
{
  struct free_chunk *p;
  p = (struct free_chunk *) malloc(bytes);
  HEADER_COMPOSE(&(p->header), bytes >> LOG_BYTES_IN_JSVALUE, HTAG_FREE, NULL);
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
      HEADER_COMPOSE((HeaderCell *) addr, alloc_jsvalues, type, NULL);
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
  compaction(ctx);
}

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
#ifdef GC_DEBUG
  sanity_check();
  check_invariant();
#endif /* GC_DEBUG */

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
    header_word_t size = HEADER_GET_SIZE(header);

    if (is_marked_cell_header(header)) {
#ifdef GC_DEBUG
      assert(HEADER_GET_MAGIC(header) == HEADER_MAGIC);
#endif /* GC_DEBUG */

      unmark_cell_header(header);
      HEADER_SET_FWD(header, HEADERPTR_TO_VALPTR(fwd));
      fwd += (size << LOG_BYTES_IN_JSVALUE);
    }
    else {
//      HEADER_SET_TYPE(header, HTAG_FREE);
      HEADER_SET_FWD(header, NULL);
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
    size_t size = HEADER_GET_SIZE(header);

    if (HEADER_GET_FWD(header) != NULL) {
      JSValue* fwd = (JSValue *)VALPTR_TO_HEADERPTR(HEADER_GET_FWD(header));
      cell_type_t type = HEADER_GET_TYPE(header);
#ifdef GC_DEBUG
      assert(HEADER_GET_MAGIC(header) == HEADER_MAGIC);
#endif /* GC_DEBUG */

      HEADER_COMPOSE(header, size, type, NULL);
#ifdef GC_DEBUG
      HEADER_SET_MAGIC(header, HEADER_MAGIC);
#endif
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
  HEADER_COMPOSE(&(tail->header), freesize, HTAG_FREE, NULL);
#ifdef GC_DEBUG
  HEADER_SET_MAGIC(&(tail->header), HEADER_MAGIC);
#endif
  tail->next = NULL;
  js_space.freelist = tail;
  js_space.free_bytes = freesize << LOG_BYTES_IN_JSVALUE;
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
      mark_cell_header(hdrp);
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

  if (get_tag(ptr) != 0) {
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
    update_JSValue((JSValue *) ptrp);
    return;
  }
}

/* Local Variables:      */
/* mode: c               */
/* c-basic-offset: 2     */
/* indent-tabs-mode: nil */
/* End:                  */
