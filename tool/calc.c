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
  int N, i;
  double *total, *gc;
  double ttotal, tgc;
  int count;

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
  if(total == NULL || gc == NULL) {
    printf("Error : memory error\n");
    if (total != NULL) free(total);
    if (gc != NULL) free(gc);
    return 2;
  }

  while(i < N) {
    if (fscanf(stdin, "%lf,%lf,%d)", &ttotal, &tgc, &count) != 3) {
      printf("Error : unknown format\n");
      break;
    }

    total[i] = ttotal;
    gc[i] = tgc;
    ++i;
  }

  if (i == N) {
    double min, first, center, third, max;
    double rawavr, avr;

    sort(total, N);
    sort(gc, N);

    getmin(total, N, &min);
    getfirst(total, N, &first);
    getcenter(total, N, &center);
    getthird(total, N, &third);
    getmax(total, N, &max);
    getrawavr(total, N, &rawavr);
    getavr(total, N, &avr);

    printf("total time [ %.1f, %.1f, %.1f, %.1f, %.1f ]", min, first, center, third, max);
    printf(" %.2f, %.2f\n", rawavr, avr);

    getmin(gc, N, &min);
    getfirst(gc, N, &first);
    getcenter(gc, N, &center);
    getthird(gc, N, &third);
    getmax(gc, N, &max);
    getrawavr(gc, N, &rawavr);
    getavr(gc, N, &avr);

    printf("GC time    [ %.1f, %.1f, %.1f, %.1f, %.1f ]", min, first, center, third, max);
    printf(" %.2f, %.2f\n", rawavr, avr);
  }

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
