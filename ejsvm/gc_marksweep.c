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
 * If the remaining room is smaller than a certain size,
 * we do not use the remainder for efficiency.  Rather,
 * we add it below the chunk being allocated.  In this case,
 * the size in the header includes the extra words.
 */
#define MINIMUM_FREE_CHUNK_JSVALUES 4

/*
 * prototype
 */
STATIC void sweep(void);

/*
 *  Space
 */
STATIC void create_space(struct space *space, size_t bytes, char *name)
{
  struct free_chunk *p;
  p = (struct free_chunk *) malloc(bytes);
  HEADER_COMPOSE(&(p->header), bytes >> LOG_BYTES_IN_JSVALUE, 0, HTAG_FREE);
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
        HEADER_COMPOSE((HeaderCell *) addr, alloc_jsvalues, 0, type);
#ifdef GC_DEBUG
        HEADER_SET_MAGIC((HeaderCell *) addr, HEADER_MAGIC);
        HEADER_SET_GEN_MASK((HeaderCell *) addr, generation);
#endif /* GC_DEBUG */
        space->free_bytes -= alloc_jsvalues << LOG_BYTES_IN_JSVALUE;
        return HEADERPTR_TO_VALPTR(addr);
      } else {
        /* This chunk is too small to split. */
        *p = (*p)->next;
        HEADER_COMPOSE(&(chunk->header), chunk_jsvalues,
                      chunk_jsvalues - alloc_jsvalues, type);
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
STATIC void collect(Context *ctx) {
  scan_roots(ctx);
  weak_clear();
  sweep();
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
        HEADER_COMPOSE(&(chunk->header), (scan - free_start) >> LOG_BYTES_IN_JSVALUE,
                      0, HTAG_FREE);
#ifdef GC_DEBUG
        HEADER_SET_MAGIC(&(chunk->header), HEADER_MAGIC);
#endif /* GC_DEBUG */
        *p = chunk;
        p = &chunk->next;
        free_bytes += scan - free_start;
      } else  {
        HEADER_COMPOSE((HeaderCell *) free_start, (scan - free_start) >> LOG_BYTES_IN_JSVALUE,
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

/* Local Variables:      */
/* mode: c               */
/* c-basic-offset: 2     */
/* indent-tabs-mode: nil */
/* End:                  */
