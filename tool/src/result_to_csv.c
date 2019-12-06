#include <stdio.h>
#include <stdlib.h>
#include <math.h>

typedef struct value {
  double total_CPU_time;
  double total_GC_time;
  double max_GC_time;
  double non_CPU_time;
  double avr_GC_time;
  size_t GC_count;
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
  if (sscanf(buf, "total CPU time = %lf msec, total GC time = %lf msec, max GC time = %lf msec (#GC = %zu)",
      &pval->total_CPU_time, &pval->total_GC_time, &pval->max_GC_time, &pval->GC_count) == 4) {
    pval->non_CPU_time = pval->total_CPU_time - pval->total_GC_time;
    pval->avr_GC_time = pval->total_GC_time / pval->GC_count;
    return 1;
  }

  return 0;
}

int scan_val_2(char *buf, Value *pval)
{
  if (sscanf(buf, "GC time out = %lf msec, max GC time = %lf msec (#GC = %zu)",
      &pval->total_GC_time, &pval->max_GC_time, &pval->GC_count) == 3) {
    pval->total_CPU_time = NAN;
    pval->non_CPU_time = pval->total_CPU_time - pval->total_GC_time;
    pval->avr_GC_time = pval->total_GC_time / pval->GC_count;
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
    if (scan_val_2(buf, pval)) {
      free(buf);
      return 1;
    }
    free(buf);
    return -1;
  }
  free(buf);


  buf = NULL;
  bufsize = 0;
  result = getline(&buf, &bufsize, fp);
  if (buf == NULL || result == -1) {
    if (buf != NULL) free(buf);
    return -1;
  }

  {
    size_t dummy1, dummy2, dummy3;

    if (sscanf(buf, "n_hc = %zu, n_enter_hc = %zu, n_exit_hc = %zu",
        &dummy1, &dummy2, &dummy3) == 3) {
      free(buf);
      return 1;
    }
  }
  free(buf);

  return -1;
}

void convert(size_t N, FILE *fp)
{
  printf("#");
  printf("%s, ", "total_CPU_time");
  printf("%s, ", "total_GC_time");
  printf("%s, ", "non_CPU_time");
  printf("%s, ", "max_GC_time");
  printf("%s, ", "avr_GC_time");
  printf("%s\n", "GC_count");

  while (1) {
    Value val;
    int result;
    
    result = get_value(fp, &val);
    if (result < 0) return;

    printf("%lf, ", val.total_CPU_time);
    printf("%lf, ", val.total_GC_time);
    printf("%lf, ", val.non_CPU_time);
    printf("%lf, ", val.max_GC_time);
    printf("%lf, ", val.avr_GC_time);
    printf("%zu\n", val.GC_count);

    if (result == 0) break;
  }
}
