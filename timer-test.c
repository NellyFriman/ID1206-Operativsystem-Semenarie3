#include <stdio.h>
#include "green.h"

long count = 0;

void *test(void *arg){
  int i = *(int*)arg;
  int loop = 100000000;
  while(loop > 0){
    //printf("thread %d: %d\n",i, loop );
    count++;
    loop--;
  //  green_yield();
  }
}

int main(){
  green_t g0, g1;
  int a0 = 0;
  int a1 = 1;
  green_create(&g0, test, &a0);
  green_create(&g1, test, &a1);

  green_join(&g0);
  green_join(&g1);
  printf("all done and the final count is %ld\n", count);
  return 0;
}
