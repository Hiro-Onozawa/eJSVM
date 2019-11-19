#include <stdio.h>
#include <stdlib.h>

void sort(double *arr, int N);
void getmin(double *arr, int N, double *ret);
void getfirst(double *arr, int N, double *ret);
void getcenter(double *arr, int N, double *ret);
void getthird(double *arr, int N, double *ret);
void getmax(double *arr, int N, double *ret);
void getrawavr(double *arr, int N, double *ret);
void getavr(double *arr, int N, double *ret);

int main(int argc, char **argv)
{
  int N, i, t, line;
  double *total, *gc;
  double ttotal, tgc;
  double *timeout;
  int count, c;

  if (argc < 1) {
    printf("usage : %s <item count>\n", argv[0]);
    return 0;
  }

  N = atoi(argv[1]);
  if (N <= 0) {
    printf("Error : <item count> is non-positive value (%d)\n", N);
    return 1;
  }

  total = (double *) malloc(sizeof(double) * N);
  gc = (double *) malloc(sizeof(double) * N);
  timeout = (double *) malloc(sizeof(double) * N);
  if(total == NULL || gc == NULL || timeout == NULL) {
    printf("Error : memory error\n");
    if (total != NULL) free(total);
    if (gc != NULL) free(gc);
    if (timeout != NULL) free(timeout);
    return 2;
  }

  count = -1;
  i = 0;
  t = 0;
  line = 0;
  while(i + t < N) {
    char *buf;
    size_t bufsize, result;

    buf = NULL;
    bufsize = 0;
    result = getline(&buf, &bufsize, stdin);
    if (buf == NULL || result == -1) {
      if (buf != NULL) free(buf);
      break;
    }
    ++line;
    
    if (sscanf(buf, "total CPU time = %lf msec, total GC time =  %lf msec (#GC = %d)", &ttotal, &tgc, &c) == 3) {
      if (count == -1) count = c;
      else if (count != c) {
        printf("Warn : gc count is not same. [%d vs %d]\n", count, c);
      }

      total[i] = ttotal;
      gc[i] = tgc;
      ++i;
    }
    else if (sscanf(buf, "GC time out =  %lf msec (#GC = %d)", &tgc, &c) == 2) {
      timeout[t] = (double) c;
      ++t;
    }
    else {
      int dummy[3];
      if (sscanf(buf, "n_hc = %d, n_enter_hc = %d, n_exit_hc = %d", dummy, dummy+1, dummy+2) != 3) {
        printf("line %d; Error : unknown format\n");
        free(buf);
        i = -1;
        break;
      }
    }
    free(buf);
  }

  if (t > 0) {
    double min, first, center, third, max;
    double rawavr, avr;

    sort(timeout, t);

    getmin(timeout, t, &min);
    getfirst(timeout, t, &first);
    getcenter(timeout, t, &center);
    getthird(timeout, t, &third);
    getmax(timeout, t, &max);
    getrawavr(timeout, t, &rawavr);
    getavr(timeout, t, &avr);

    printf("timeout GC count [ %.1f, %.1f, %.1f, %.1f, %.1f ]", min, first, center, third, max);
    printf(" %.2f, %.2f\n", rawavr, avr);
  }
  if (i > 0) {
    double min, first, center, third, max;
    double rawavr, avr;

    sort(total, i);
    sort(gc, i);

    getmin(total, i, &min);
    getfirst(total, i, &first);
    getcenter(total, i, &center);
    getthird(total, i, &third);
    getmax(total, i, &max);
    getrawavr(total, i, &rawavr);
    getavr(total, i, &avr);

    printf("total time [ %.1f, %.1f, %.1f, %.1f, %.1f ]", min, first, center, third, max);
    printf(" %.2f, %.2f\n", rawavr, avr);

    getmin(gc, i, &min);
    getfirst(gc, i, &first);
    getcenter(gc, i, &center);
    getthird(gc, i, &third);
    getmax(gc, i, &max);
    getrawavr(gc, i, &rawavr);
    getavr(gc, i, &avr);

    printf("GC time    [ %.1f, %.1f, %.1f, %.1f, %.1f ]", min, first, center, third, max);
    printf(" %.2f, %.2f\n", rawavr, avr);

    printf("GC count : %d\n", count);
  }

  free(timeout);
  free(gc);
  free(total);
}

void sort(double *arr, int N)
{
  int i, j;
  for (i = 0; i < N; ++i) {
    for (j = i + 1; j < N; ++j) {
      double a = arr[i];
      double b = arr[j];
      if (a < b) { }
      else {
        arr[i] = b;
        arr[j] = a;
      }
    }
  }
}

void getmin(double *arr, int N, double *ret)
{
  *ret = arr[0];
}

void getfirst(double *arr, int N, double *ret)
{
  if (N % 2 == 0) {
    getcenter(arr, N / 2, ret);
  }
  else {
    getcenter(arr, (N - 1) / 2, ret);
  }
}

void getcenter(double *arr, int N, double *ret)
{
  if (N % 2 == 0) {
    double small, big;
    small = arr[(N / 2) - 1];
    big = arr[N / 2];
    *ret = (small + big) / 2.0;
  }
  else {
    *ret = arr[(N - 1) / 2];
  }
}

void getthird(double *arr, int N, double *ret)
{
  if (N % 2 == 0) {
    getcenter(arr + (N / 2), N / 2, ret);
  }
  else {
    getcenter(arr + ((N - 1) / 2) + 1, (N - 1) / 2, ret);
  }
}

void getmax(double *arr, int N, double *ret)
{
  *ret = arr[N - 1];
}

void getrawavr(double *arr, int N, double *ret)
{
  double sum;
  int i;

  sum = 0.0;
  for (i = 0; i < N; ++i) {
    sum += arr[i];
  }
  *ret = sum / (double) N;
}

void getavr(double *arr, int N, double *ret)
{
  int min, max;

  min = 0 + (N / 4);
  max = N - (N / 4);

  getrawavr(arr + min, (max - min), ret);
}
