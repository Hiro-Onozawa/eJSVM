#include <stdio.h>
#include <stdlib.h>

typedef struct value {
  size_t before_collect;
  size_t after_collect;
  size_t garbage;
  double collect_rate;
  size_t alive_object_count;
  size_t alloced_object_count;
  double move_object_rate;
  size_t move_size;
  double move_size_rate;
} Value;

void convert(size_t N, FILE *fp);
int get_value(FILE *fp, Value *pval);

int main(int argc, char **argv) {
  size_t N;
  FILE *fp;

  if (argc < 2) {
    printf("usage : %s <max item count> <file path>\n", argv[0]);
    return 0;
  }

  N = atoi(argv[1]);
  if (N <= 0) {
    printf("Error : <item count> is non-positive value (%d)\n", N);
    return 1;
  }

  fp = fopen(argv[2], "r");
  if (fp == NULL) {
    printf("Error : file \"%s\" can not open.\n", argv[2]);
    return 2;
  }

  convert(N, fp);

  fclose(fp);

  return 0;
}

int scan_val_1(char *buf, Value *pval)
{
  if (sscanf(buf, "allocated bytes : %zu to %zu : %zu bytes ( %lf %%) collected",
      &pval->before_collect, &pval->after_collect, &pval->garbage, &pval->collect_rate) == 4) {
    return 1;
  }

  return 0;
}

int scan_val_2(char *buf, Value *pval)
{
  if (sscanf(buf, "%zu / %zu ( %lf %%) objects moved : %zu bytes ( %lf %%)",
      &pval->alive_object_count, &pval->alloced_object_count, &pval->move_object_rate,
      &pval->move_size, &pval->move_size_rate) == 5) {
    return 1;
  }
  
  return 0;
}

int get_value(FILE *fp, Value *pval)
{
  char *buf;
  size_t bufsize, result;

  buf = NULL;
  bufsize = 0;
  result = getline(&buf, &bufsize, fp);
  if (buf == NULL || result == -1) {
    if (buf != NULL) free(buf);
    return -1;
  }

  if (!scan_val_1(buf, pval)) {
    double dummy1, dummy2;
    size_t dummy3;

    if (sscanf(buf, "total CPU time = %lf msec, total GC time =  %lf msec (#GC = %zu)",
        &dummy1, &dummy2, &dummy3) == 3) {
      free(buf);
      return 0;
    }
  }
  free(buf);
    

  buf = NULL;
  bufsize = 0;
  result = getline(&buf, &bufsize, fp);
  if (buf == NULL || result == -1) {
    if (buf != NULL) free(buf);
    return -1;
  }

  if (scan_val_2(buf, pval)) {
    free(buf);
    return 1;
  }
  free(buf);

  return -1;
}

void convert(size_t N, FILE *fp)
{
  printf("#");
  printf("%s, ", "before_collect");
  printf("%s, ", "after_collect");
  printf("%s, ", "garbage");
  printf("%s, ", "collect_rate");
  printf("%s, ", "alive_object_count");
  printf("%s, ", "alloced_object_count");
  printf("%s, ", "move_object_rate");
  printf("%s, ", "move_size");
  printf("%s\n", "move_size_rate");

  while (1) {
    Value val;
    int result;
    
    result = get_value(fp, &val);
    if (result < 0) return;

    printf("%zu, ", val.before_collect);
    printf("%zu, ", val.after_collect);
    printf("%zu, ", val.garbage);
    printf("%lf, ", val.collect_rate);
    printf("%zu, ", val.alive_object_count);
    printf("%zu, ", val.alloced_object_count);
    printf("%lf, ", val.move_object_rate);
    printf("%zu, ", val.move_size);
    printf("%lf\n", val.move_size_rate);

    if (result == 0) break;
  }
}
