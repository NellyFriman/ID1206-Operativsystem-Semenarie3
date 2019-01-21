#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>

void *test(void *arg){
  int i = *(int*)arg;
  int loop = 4;
  while(loop > 0){
    //printf("thread %d: %d\n",i, loop );
    loop--;
  //  sched_yield();
  }
}

int main(){

  int len = 100;
  pthread_t g[len];
  int a[len];


for (int i = 0; i < len; i++) {
  a[i] = i;
  pthread_create(&g[i],NULL, test, &a[i]);
}
  pthread_join(g[len-1], NULL);



  /*
  green_join(&g0);
  green_join(&g1);
  green_join(&g2);
  */
  printf("all done\n");
  return 0;
}
