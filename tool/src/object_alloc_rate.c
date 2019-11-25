#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct datfile_tag {
  char *name;
  FILE *fp;
} DatFile;

typedef struct params_tag {
  int alloccnt;
  int request;
  int alloc;
  int header;
  int waste;
} Params;

void read_params(FILE *fp, Params *params, size_t N);

int main(int argc, char **argv)
{
  size_t N;
  int idx, is_rate;
  DatFile *fparr;
  size_t file_cnt;
  int i, j;
  size_t t;
  Params params[32];

  if (argc < 2) {
    printf("usage : %s <param index> [ <file path> <label> ] ...\n", argv[0]);
    printf("param index needs to belong to\n");
    printf("   0 : allocated times\n");
    printf("   1 : request bytes\n");
    printf("   2 : allocated bytes\n");
    printf("   3 : header bytes\n");
    printf("   4 : waste bytes\n");
    printf("  10 : allocated times (rate)\n");
    printf("  11 : request bytes (rate)\n");
    printf("  12 : allocated bytes (rate)\n");
    printf("  13 : header bytes (rate)\n");
    printf("  14 : waste bytes (rate)\n");
    return 0;
  }

  idx = atoi(argv[1]);
  is_rate = idx / 10;
  idx = idx % 10;
  if ((idx < 0 || 4 < idx) || (is_rate != 0 && is_rate != 1)) {
    printf("Error : Unknown <param index> (%d)\n", idx);
    return 1;
  }

  fparr = (DatFile *) malloc(sizeof(DatFile) * (argc - 1) / 2);
  if (fparr == NULL) {
    printf("Error : Memory error\n");
    return 2;
  }
  for (i = 0; i < (argc - 1) / 2; ++i) {
    fparr[i].fp = NULL;
    fparr[i].name = NULL;
  }

  for (i = 2, j = 0; i < argc; i += 2) {
    fparr[j].fp = fopen(argv[i], "r");
    if (fparr[j].fp == NULL) {
      printf("Error : file \"%s\" can not open.\n", argv[i]);
      continue;
    }
    fparr[j].name = argv[i + 1];
    ++j;
  }
  file_cnt = (size_t) j;
  if (file_cnt == 0) {
    printf("Error : No valid file pointer.\n");
    free(fparr);
    return 3;
  }

  printf("# label, STRING, FLONUM, OBJECT, ARRAY, FUNCTION, BUILTIN, ITERATOR, BOX_STRING, BOX_NUMBER, BOX_BOOLEAN, PROP, ARRAY_DATA, FUNCTION_FRAME, HASH_BODY, STR_CONS, STACK, HIDDEN_CLASS\n");

  for (t = 0; t < file_cnt; ++t) {
    int total;

    read_params(fparr[t].fp, params, 32);

    total = 0;
    for (i = 0; i < 17; ++i) {
      int val;

      switch(idx) {
        case 0 : val = params[i].alloccnt; break;
        case 1 : val = params[i].request; break;
        case 2 : val = params[i].alloc; break;
        case 3 : val = params[i].header; break;
        case 4 : val = params[i].waste; break;
      }
      total += val;
    }

    printf("%s", fparr[t].name);
    for (i = 0; i < 17; ++i) {
      int val;
      double rate;

      switch(idx) {
        case 0 : val = params[i].alloccnt; break;
        case 1 : val = params[i].request; break;
        case 2 : val = params[i].alloc; break;
        case 3 : val = params[i].header; break;
        case 4 : val = params[i].waste; break;
      }

      rate = (double) val / (double) total;
      if (is_rate) printf(" %.6f", rate);
      else printf(" %d", val);
    }
    printf("\n");
  }

  for (t = 0; t < file_cnt; ++t) {
    if (fparr[t].fp != NULL) fclose(fparr[t].fp);
    fparr[t].fp = NULL;
    fparr[t].name = NULL;
  }

  free(fparr);
}

void read_params(FILE *fp, Params *params, size_t N) {
  int line;
  size_t i;

  for (i = 0; i < N; ++i) {
    params[i].alloccnt = 0;
    params[i].request = 0;
    params[i].alloc = 0;
    params[i].header = 0;
    params[i].waste = 0;
  }

  line = 0;
  while(1) {
    char *buf;
    size_t bufsize, result;
    char typename[32];
    Params param;

    buf = NULL;
    bufsize = 0;
    result = getline(&buf, &bufsize, fp);
    if (buf == NULL || result == -1) {
      if (buf != NULL) free(buf);
      break;
    }
    ++line;
    
    if (buf[0] != 't' || buf[1] != 'y' || buf[2] != 'p' || buf[3] != 'e') {
    } else if (
      sscanf(
        buf,
        "type %s : allocated %d times, request %d bytes, allocated %d bytes, header %d bytes, waste %d bytes",
        typename, &param.alloccnt, &param.request, &param.alloc, &param.header, &param.waste
      ) == 6
    ) {
      int idx;

      if (strcmp(typename, "STRING") == 0)              idx =  0;
      else if (strcmp(typename, "FLONUM") == 0)         idx =  1;
      else if (strcmp(typename, "OBJECT") == 0)         idx =  2;
      else if (strcmp(typename, "ARRAY") == 0)          idx =  3;
      else if (strcmp(typename, "FUNCTION") == 0)       idx =  4;
      else if (strcmp(typename, "BUILTIN") == 0)        idx =  5;
      else if (strcmp(typename, "ITERATOR") == 0)       idx =  6;
      else if (strcmp(typename, "BOX_STRING") == 0)     idx =  7;
      else if (strcmp(typename, "BOX_NUMBER") == 0)     idx =  8;
      else if (strcmp(typename, "BOX_BOOLEAN") == 0)    idx =  9;
      else if (strcmp(typename, "PROP") == 0)           idx = 10;
      else if (strcmp(typename, "ARRAY_DATA") == 0)     idx = 11;
      else if (strcmp(typename, "FUNCTION_FRAME") == 0) idx = 12;
      else if (strcmp(typename, "HASH_BODY") == 0)      idx = 13;
      else if (strcmp(typename, "STR_CONS") == 0)       idx = 14;
      else if (strcmp(typename, "STACK") == 0)          idx = 15;
      else if (strcmp(typename, "HIDDEN_CLASS") == 0)   idx = 16;
      else idx = -1;

      if (idx < 0) {
        fprintf(stderr, "line %d; Error : Unknown typename \"%s\"\n", typename);
      }
      else {
        params[idx] = param;
      }
    }
    else {
    }
    free(buf);
  }
}