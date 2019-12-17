/*
 * eJS Project
 * Kochi University of Technology
 * The University of Electro-communications
 *
 * The eJS Project is the successor of the SSJS Project at The University of
 * Electro-communications.
 */

/*
 *  Types
 */
struct space {
  uintptr_t addr;
  uintptr_t free;
  uintptr_t toSpace;
  uintptr_t fromSpace;
  uintptr_t top;
  size_t extent;
  size_t free_bytes;
  char *name;
};

/*
 * prototype
 */
STATIC void collect(Context *ctx);

/*
 *  Space
 */
STATIC void create_space(struct space *space, size_t bytes, char *name)
{
  void *p;
  size_t extent;

  p = malloc(bytes);
  assert(p != NULL);

  extent = bytes / 2;
  HEADER_COMPOSE(p, extent, HTAG_FREE);

  space->addr       = (uintptr_t) p;
  space->free       = (uintptr_t) p;
  space->toSpace    = (uintptr_t) p;
  space->fromSpace  = ((uintptr_t) p) + extent;
  space->top        = ((uintptr_t) p) + extent;
  space->extent     = extent;
  space->free_bytes = extent;
  space->name       = name;
}

STATIC int in_js_to_space(void *addr_)
{
  uintptr_t addr = (uintptr_t) addr_;
  return (js_space.toSpace <= addr && addr < js_space.top);
}

STATIC int in_js_from_space(void *addr_)
{
  uintptr_t addr = (uintptr_t) addr_;
  return (js_space.fromSpace <= addr && addr < js_space.fromSpace + js_space.extent);
}

STATIC int in_js_space(void *addr_)
{
  uintptr_t addr = (uintptr_t) addr_;
  return (js_space.addr <= addr && addr < js_space.addr + js_space.extent * 2);
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

STATIC void* space_alloc(struct space *space,
                         size_t request_bytes, cell_type_t type)
{
  size_t alloc_jsvalues;
  HeaderCell *hdrp;
  uintptr_t newfree;
  
  alloc_jsvalues =
    (request_bytes + BYTES_IN_JSVALUE - 1) >> LOG_BYTES_IN_JSVALUE;
  alloc_jsvalues += HEADER_JSVALUES;
  
  hdrp = (HeaderCell *) space->free;
  newfree = space->free + (alloc_jsvalues << LOG_BYTES_IN_JSVALUE);

  if (newfree > space->top) {
    printf("memory exhausted\n"); fflush(stdout);
    return NULL;
  }
  space->free = newfree;
  space->free_bytes -= alloc_jsvalues << LOG_BYTES_IN_JSVALUE;

  HEADER_COMPOSE(hdrp, alloc_jsvalues, type);
  HEADER_COMPOSE(newfree, space->free_bytes >> LOG_BYTES_IN_JSVALUE, HTAG_FREE);
#ifdef GC_DEBUG
  HEADER_SET_MAGIC(hdrp, HEADER_MAGIC);
  HEADER_SET_GEN_MASK(hdrp, generation);
  HEADER_SET_MAGIC((HeaderCell *) newfree, HEADER_MAGIC);
#endif /* GC_DEBUG */

  return (void *) HEADERPTR_TO_VALPTR(hdrp);
}


/*
 * GC
 */
STATIC void *worklist;

STATIC void flip();
STATIC void wl_initialize(void **worklist);
STATIC int  wl_isEmpty(void **worklist);
STATIC void wl_add(void **worklist, void *ref);
STATIC void *wl_remove(void **worklist);
STATIC void scan_HiddenClass(HiddenClass *phc);
STATIC void scan_StrCons(StrCons **pstrcons);
STATIC void scan_StrCons_ptr_array(StrCons **ptrarr, size_t size);
STATIC void scan_StrTable(StrTable *pstrtable);
STATIC void scan_HashCell(HashCell *hash);
STATIC void scan_HashBody(HashCell **pbody);
STATIC void scan_FnctionFrame(FunctionFrame *pfframe);
STATIC void scan_BoxedCell(BoxedCell *pcell);
STATIC void scan_Iterator(Iterator *pitr);
STATIC void scan_FunctionCell(FunctionCell *pfunc);
STATIC void scan_ArrayCell(ArrayCell *parr);
STATIC void scan_ObjectCell(Object *pobj);
STATIC void scan(void *ref);
STATIC void process_JSValue(JSValue *pjsv);
STATIC void process_JSValue_array(JSValue *jsvarr, size_t len);
STATIC void process_StrCons_ptr_array(StrCons **pscarr, size_t len);
STATIC void process_Context(Context *ctx);
STATIC void process_stack(JSValue **pstack, int sp, int fp);
STATIC void update_regbase(intptr_t diff);
STATIC void process_FunctionTable(FunctionTable *pft);
STATIC void process_FunctionTable_array(FunctionTable *ftarr, size_t len);
STATIC void process_root_ptr(void **ptrp);
STATIC void process(void **pref);
STATIC void process_roots(Context *ctx);
STATIC void *forwardingAddress(void *fromRef);
STATIC void *forward(void *fromRef);
STATIC void *copy(void *fromRef);

STATIC void flip()
{
  uintptr_t tmp = js_space.toSpace;
  js_space.toSpace = js_space.fromSpace;
  js_space.fromSpace = tmp;

  js_space.top = js_space.toSpace + js_space.extent;
  js_space.free = js_space.toSpace;
  js_space.free_bytes = js_space.extent;
}

STATIC void wl_initialize(void **worklist)
{
  *worklist = (void *) js_space.free;
}

STATIC int wl_isEmpty(void **worklist)
{
  return *worklist == (void *) js_space.free;
}

STATIC void wl_add(void **worklist, void *ref)
{
  // Nothing to do
}

STATIC void *wl_remove(void **worklist)
{
  void *ref;
  HeaderCell *hdrp;
  size_t size;

  ref = *worklist;

  hdrp = (HeaderCell *) *worklist;
  size = (HEADER_GET_SIZE(hdrp) << LOG_BYTES_IN_JSVALUE);
  *worklist = (void *) (((uintptr_t) *worklist) + size);

  return ref;
}

STATIC void scan_ObjectCell(Object *pobj)
{
  process((void **) &(pobj->class));
  process((void **) &(pobj->prop));
}

STATIC void scan_ArrayCell(ArrayCell *parr)
{
  scan_ObjectCell(&(parr->o));
  process((void **) &(parr->body));
}

STATIC void scan_FunctionCell(FunctionCell *pfunc)
{
  scan_ObjectCell(&(pfunc->o));
  assert(!in_js_space(pfunc->func_table_entry));
  process((void **) &(pfunc->environment));
}

STATIC void scan_Iterator(Iterator *pitr)
{
  process((void **) &(pitr->body));
}

STATIC void scan_BoxedCell(BoxedCell *pcell)
{
  scan_ObjectCell(&(pcell->o));
  process_JSValue(&(pcell->value));
}

STATIC void scan_FnctionFrame(FunctionFrame *pfframe)
{
  HeaderCell *hdrp;
  JSValue *head;
  JSValue *end;
  size_t n_locals;

  process((void **) &(pfframe->prev_frame));

  hdrp = VALPTR_TO_HEADERPTR(pfframe);
  head = &(pfframe->arguments);
  end = ((JSValue *) hdrp) + HEADER_GET_SIZE(hdrp);
  n_locals = ((uintptr_t)end - (uintptr_t)head) / sizeof(JSValue);
  process_JSValue_array(head, n_locals);
}

STATIC void scan_HashBody(HashCell **pbody)
{
  HeaderCell *hdrp;
  header_word_t size;
  uintptr_t end;
  size_t len;

  hdrp = VALPTR_TO_HEADERPTR(pbody);
  size = HEADER_GET_SIZE(hdrp);
  end = (uintptr_t)((JSValue *) hdrp + size);
  len = (size_t)(end - (uintptr_t) pbody) / sizeof(HashCell *);

  while (len > 0) {
    if (*pbody != NULL) scan_HashCell(*pbody);
    ++pbody;
    --len;
  }
}

STATIC void scan_HashCell(HashCell *hash)
{
  assert(!in_js_space(hash));

  process_JSValue((JSValue *) &(hash->entry.key));

  if (is_transition(hash->entry.attr)) {
    process((void **) ((HiddenClass **) &(hash->entry.data)));
  }

  if (hash->next != NULL) {
    scan_HashCell(hash->next);
  }
}

STATIC void scan_StrTable(StrTable *pstrtable)
{
  scan_StrCons_ptr_array(pstrtable->obvector, pstrtable->size);
}

STATIC void scan_StrCons_ptr_array(StrCons **ptrarr, size_t size)
{
  size_t i;

  for (i = 0; i < size; ++i) {
    StrCons **pstrcons;

    pstrcons = ptrarr + i;
    if (*pstrcons != NULL) {
      scan_StrCons(pstrcons);
    }
  }
}

STATIC void scan_StrCons(StrCons **pstrcons)
{
  StringCell *cell = remove_normal_string_tag((*pstrcons)->str);
  void *toRef = forwardingAddress(cell);

  if (toRef != NULL) {
    (*pstrcons)->str = put_tag(toRef, T_STRING);
    process((void **) pstrcons);
    pstrcons = &((*pstrcons)->next);
  }
  else {
    *pstrcons = (*pstrcons)->next;
  }
  if (*pstrcons != NULL) scan_StrCons(pstrcons);
}

STATIC void scan_HiddenClass(HiddenClass *phc)
{
  HashTable *map;

  map = phc->map;
  assert(!in_js_space(map));

  process((void **) &(map->body));
}

STATIC void scan(void *ref)
{
  HeaderCell *hdrp;
  void *ptr;
  header_word_t size;
  header_word_t type;

  assert(in_js_to_space(ref));

  hdrp = (HeaderCell *) ref;
  size = HEADER_GET_SIZE(hdrp);
  type = HEADER_GET_TYPE(hdrp);
  ptr = HEADERPTR_TO_VALPTR(hdrp);

  switch(type) {
    case HTAG_FREE:
      LOG_EXIT("scan : htag is HTAG_FREE\n");
      return;

    case HTAG_STRING:
    case HTAG_FLONUM:
      // Nothing to do
      break;

    case HTAG_SIMPLE_OBJECT:
      scan_ObjectCell((Object *) ptr);
      break;
    case HTAG_ARRAY:
      scan_ArrayCell((ArrayCell *) ptr);
      break;
    case HTAG_FUNCTION:
      scan_FunctionCell((FunctionCell *) ptr);
      break;
    case HTAG_BUILTIN:
      // Same as ObjectCell
      scan_ObjectCell((Object *) ptr);
      break;
    case HTAG_ITERATOR:
      scan_Iterator((Iterator *) ptr);
      break;
#ifdef use_regexp
    case HTAG_REGEXP:
      LOG_EXIT("Not implemented\n");
      return;
#endif
    case HTAG_BOXED_STRING:
    case HTAG_BOXED_NUMBER:
    case HTAG_BOXED_BOOLEAN:
      scan_BoxedCell((BoxedCell *) ptr);
      break;

    case HTAG_PROP:
    case HTAG_ARRAY_DATA:
      process_JSValue_array((JSValue *) ptr, size - HEADER_JSVALUES);
      break;
    case HTAG_FUNCTION_FRAME:
      scan_FnctionFrame((FunctionFrame *) ptr);
      break;
    case HTAG_HASH_BODY:
      scan_HashBody((HashCell**) ptr);
      break;
    case HTAG_STR_CONS:
      // StrCons should be scaned after that scan heap.
      break;
    case HTAG_CONTEXT:
      LOG_EXIT("Unreachable Code\n");
      abort();
      break;
    case HTAG_STACK:
      // Nothing to do
      break;
    case HTAG_HIDDEN_CLASS:
      scan_HiddenClass((HiddenClass *) ptr);
      break;
    
    default:
     LOG_EXIT("Unknown tag : 0x%04"PRIJSValue"\n", HEADER_GET_TYPE(hdrp));
     abort();
  }
}

STATIC void process_JSValue(JSValue *pjsv)
{
  JSValue jsv;
  Tag tag;
  void *fromRef;
  void *toRef;

  jsv = *pjsv;
  if (!is_pointer(jsv)) return;

  tag = get_tag(jsv);
  fromRef = (void *) clear_tag(jsv);

  toRef = forward(fromRef);

  *pjsv = put_tag(toRef, tag);
}

STATIC void process_JSValue_array(JSValue *jsvarr, size_t len)
{
  size_t i;
  for (i = 0; i < len; ++i) {
    process_JSValue(jsvarr + i);
  }
}

STATIC void process_StrCons_ptr_array(StrCons **pscarr, size_t len)
{
  size_t i;
  for (i = 0; i < len; ++i) {
    process((void **) (pscarr + i));
  }
}

STATIC void process_Context(Context *ctx)
{
  process_JSValue(&(ctx->global));

  // Nothing to do
  assert(!in_js_space(ctx->function_table));

  process((void **) &(ctx->spreg.lp));

  process_JSValue(&(ctx->spreg.a));
  process_JSValue(&(ctx->spreg.err));

  process_JSValue(&(ctx->exhandler_stack));
  process_JSValue(&(ctx->lcall_stack));

  process_stack(&(ctx->stack), ctx->spreg.sp, ctx->spreg.fp);
}

STATIC void process_stack(JSValue **pstack, int sp, int fp)
{
  JSValue *stack = *pstack;
  void *toRef;

  assert(in_js_space(stack));
  toRef = forwardingAddress(stack);
  if (toRef != NULL) {
    *pstack = toRef;
    return;
  }

  if (sp == 0 && fp == 0) return;
  while (1) {
    while (sp >= fp) {
      process_JSValue(stack + sp);
      sp--;
    }
    if (sp < 0)
      break;

    fp = stack[sp--];                           /* FP */
    process((void **) ((FunctionFrame **) &(stack[sp--]))); /* LP */
    sp--;                                       /* PC */
    assert(!in_js_space((void *) stack[sp]));
    sp--;                                       /* CF */
    /* TODO: fixup inner pointer (CF) */
  }

  process((void **) pstack);

  intptr_t stack_old = (intptr_t)stack;
  intptr_t stack_new = (intptr_t)*pstack;
  intptr_t diff = stack_old - stack_new;
  update_regbase(diff);
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

STATIC void process_FunctionTable(FunctionTable *pft)
{
  JSValue *table;
  size_t n_constants;

  if (pft->insns == NULL) return;

  table = (JSValue *) (pft->insns + pft->n_insns);
  n_constants = (size_t) pft->n_constants;

  process_JSValue_array(table, n_constants);
}

STATIC void process_FunctionTable_array(FunctionTable *ftarr, size_t len)
{
  size_t i;
  for (i = 0; i < len; ++i) {
    FunctionTable *pft;
    pft = ftarr + i;
    if (pft != NULL) {
      process_FunctionTable(pft);
    }
  }
}

STATIC void process_root_ptr(void **ptrp)
{
  void *ptr = *ptrp;

  if (get_tag(ptr) != 0) {
    JSValue *pjsv = (JSValue *) ptrp;
    process_JSValue(pjsv);
    return;
  }

  switch (obj_header_tag(ptr)) {
  case HTAG_PROP:
    printf("HTAG_PROP in process_root_ptr\n");
    break;
  case HTAG_ARRAY_DATA:
    printf("HTAG_ARRAY_DATA in process_root_ptr\n");
    break;
  case HTAG_FUNCTION_FRAME:
    process((void **) ((FunctionFrame **) ptrp));
    break;
  case HTAG_HASH_BODY:
    process((void **) ((HashCell ***) ptrp));
    break;
  case HTAG_STR_CONS:
    printf("HTAG_STR_CONS in process_root_ptr\n");
    break;
  case HTAG_CONTEXT:
    printf("HTAG_CONTEXT in process_root_ptr\n");
    break;
  case HTAG_STACK:
    printf("HTAG_STACK in process_root_ptr\n");
    break;
#ifdef HIDDEN_CLASS
  case HTAG_HIDDEN_CLASS:
    process((void **) ((HiddenClass **) ptrp));
    break;
#endif
  default:
    process_JSValue((JSValue *) ptrp);
    return;
  }
}

STATIC void process(void **pref)
{
  void *fromRef;

  fromRef = *pref;
  if (fromRef != NULL) {
    void *toRef;
    toRef = forward(fromRef);
    *pref = toRef;
  }
}

STATIC void *forwardingAddress(void *fromRef)
{
  HeaderCell *hdrp;
  header_word_t hdr0;
  void *p;

  hdrp = VALPTR_TO_HEADERPTR(fromRef);
  hdr0 = hdrp->header0;
  p = (void *) hdr0;

  return (in_js_to_space(p))? p : NULL;
}

STATIC void *forward(void *fromRef)
{
  void *toRef;

  if (fromRef == NULL) return NULL;

  assert(in_js_from_space(fromRef));

  toRef = forwardingAddress(fromRef);
  if (toRef == NULL) {
    toRef = copy(fromRef);
  }

  return toRef;
}

STATIC void *copy(void *fromRef)
{
  HeaderCell *hdrp_from;
  size_t size;
  void *from;
  void *to;
  void *toRef;

  hdrp_from = VALPTR_TO_HEADERPTR(fromRef);
  size = HEADER_GET_SIZE(hdrp_from);
  from = (void *) hdrp_from;
  to = (void *) js_space.free;
  toRef = (void *) HEADERPTR_TO_VALPTR(to);

#ifdef GC_DEBUG
  assert(HEADER_GET_MAGIC(hdrp_from) == HEADER_MAGIC);
#endif /* GC_DEBUG */

  js_space.free = (uintptr_t) (((JSValue *) to) + size);
  js_space.free_bytes -= size << LOG_BYTES_IN_JSVALUE;

  copy_object(from, to, size);

  hdrp_from->header0 = (header_word_t) toRef;
  wl_add(&worklist, toRef);

  return toRef;
}

STATIC void process_roots(Context *ctx)
{
  int i;

  /*
   * global variables
   */
  {
    struct global_constant_objects *gconstsp = &gconsts;
    size_t len = ((uintptr_t)(gconstsp + 1) - (uintptr_t)gconstsp) / sizeof(JSValue);
    process_JSValue_array((JSValue *)gconstsp, len);
  }

  /*
   * global malloced objects
   * For simplicity, we do not use a `for' loop to visit every object
   * registered in the gobjects.
   */
#ifdef HIDDEN_CLASS
  process((void **) &(gobjects.g_hidden_class_0));
#endif

  /* function table: do not trace.
   *                 Used slots should be traced through Function objects
   */

  /* string table: do not trace.
   *               Used slot should be traced after that scan heap.
   */
//  process_StrCons_ptr_array(string_table.obvector, string_table.size);

  /*
   * Context
   */
  process_Context(ctx);

  process_FunctionTable_array(&function_table[0], FUNCTION_TABLE_LIMIT);

  /*
   * tmp root
   */
  /* old gc root stack */
  for (i = 0; i <= tmp_roots_sp; i++)
    process_root_ptr((void **)tmp_roots[i]);

  /* new gc root stack */
  for (i = 0; i < gc_root_stack_ptr; i++)
    process_root_ptr((void **)gc_root_stack[i]);
}

STATIC void collect(Context *ctx)
{
  flip();
  wl_initialize(&worklist);

  process_roots(ctx);

  while (!wl_isEmpty(&worklist)) {
    void *ref;
    ref = wl_remove(&worklist);
    scan(ref);
  }

  scan_StrTable(&string_table);
}


#ifdef GC_CLEAR_MEM
STATIC void fill_free_cell(struct space *space, JSValue val)
{
  JSValue *head;
  JSValue *end;

  head = space->fromSpace;
  end = ((uintptr_t) head) + space->extent;

  while (head < end) {
    *head = val;
    ++head;
  }

  head = space->free + HEADER_JSVALUES;
  end = space->top;

  while (head < end) {
    *head = val;
    ++head;
  }
}
#endif /* GC_CLEAR_MEM */

#ifdef GC_PROFILE
STATIC size_t gc_get_allocated_bytes()
{
  return (size_t) (js_space.extent - js_space.free_bytes);
}
#endif /* GC_PROFILE */

/* Local Variables:      */
/* mode: c               */
/* c-basic-offset: 2     */
/* indent-tabs-mode: nil */
/* End:                  */
