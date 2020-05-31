/*
 * eJS Project
 * Kochi University of Technology
 * The University of Electro-communications
 *
 * The eJS Project is the successor of the SSJS Project at The University of
 * Electro-communications.
 */

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
 *  Macro
 */

#define GC_MARK_BIT (1 << HEADER_GC_OFFSET)


/*
 * prototype
 */
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

#ifdef GC_CLEAR_MEM
STATIC void fill_free_cell(struct space *p, JSValue val);
#endif /* GC_CLEAR_MEM */


/*
 * GC
 */
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
#if (defined GC_MARK_SWEEP)
    assert(HEADER_GET_SIZE(hdrp) - HEADER_GET_EXTRA(hdrp) ==
           HEADER_GET_SIZE(shadow) - HEADER_GET_EXTRA(shadow));
#endif /* (defined GC_MARK_SWEEP) */
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
  uint32_t i;
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
  HeaderCell *hdrp;
  size_t length;
  size_t   i;
  if (test_and_mark_cell(ptr))
    return;

  if (ptr->prev_frame != NULL)
    trace_FunctionFrame(&ptr->prev_frame);
  trace_slot(&ptr->arguments);
  /* locals */
  hdrp = VALPTR_TO_HEADERPTR(ptr);
  length = HEADER_GET_SIZE(hdrp);
  length -= HEADER_JSVALUES;
  length -= sizeof(FunctionFrame) >> LOG_BYTES_IN_JSVALUE;
#if (defined GC_MARK_SWEEP)
  length -= HEADER_GET_EXTRA(hdrp);
#endif
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

  header_word_t type;
  type = HEADER_GET_TYPE(VALPTR_TO_HEADERPTR(ptr));
  switch (type) {
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
  if (sp == 0 && fp == 0) return;
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
//        HEADER_SET_TYPE(VALPTR_TO_HEADERPTR(cell), HTAG_FREE);
//        HEADER_SET_TYPE(VALPTR_TO_HEADERPTR(*p), HTAG_FREE);
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


#ifdef GC_DEBUG
STATIC void check_invariant_nobw_space(struct space *space)
{
  uintptr_t scan = space->addr;

  while (scan < space->addr + space->bytes) {
    HeaderCell *hdrp = (HeaderCell *) scan;
    header_word_t type = HEADER_GET_TYPE(hdrp);
    if (type == HTAG_STRING)
      ;
#ifdef need_flonum
    else if (type == HTAG_FLONUM)
      ;
#endif
    else if (type == HTAG_CONTEXT)
      ;
    else if (type == HTAG_STACK)
      ;
    else if (is_marked_cell_header(hdrp)) {
      /* this object is black; should not contain a pointer to white */
      size_t payload_jsvalues = HEADER_GET_SIZE(hdrp);
      size_t i;
      payload_jsvalues -= HEADER_JSVALUES;
#if (defined GC_MARK_SWEEP)
      payload_jsvalues -= HEADER_GET_EXTRA(hdrp);
#endif
      for (i = 0; i < payload_jsvalues; i++) {
        JSValue x = ((JSValue *) (scan + HEADER_BYTES))[i];
        if (type == HTAG_STR_CONS) {
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

#ifdef GC_CLEAR_MEM
STATIC void fill_free_cell(struct space *space, JSValue val)
{
  struct free_chunk *p = space->freelist;

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

#ifdef GC_PROFILE
extern int heap_limit;
STATIC size_t gc_get_allocated_bytes()
{
  return (size_t) ((uintptr_t) heap_limit - js_space.free_bytes);
}
#endif /* GC_PROFILE */

/* Local Variables:      */
/* mode: c               */
/* c-basic-offset: 2     */
/* indent-tabs-mode: nil */
/* End:                  */
