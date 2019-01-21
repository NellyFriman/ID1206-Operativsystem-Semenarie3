#include <stdio.h>
#include "green.h"

long count = 0;
green_mutex_t mutex;
int flag = 0;

void *test(void *arg){
  int i = *(int*)arg;
  int loop = 1000000;
//printf("thread %d is about to lock\n", i );
//green_mutex_lock(&mutex);
  while(loop > 0){
green_mutex_lock(&mutex);
    count++;
    loop--;
  //  printf("tread %d is counting\n",i );
green_mutex_unlock(&mutex);
   //green_yield();
  }
//green_mutex_unlock(&mutex);
}
/*void *test(void *arg){
  int id = *(int*)arg;
  int loop = 10000;
  //printf("now running: %d\n", id );
  while(loop > 0){
  //  green_mutex_lock(&mutex);
    if(flag == id){
      green_mutex_lock(&mutex);
      printf("%ld thread %d: %d\n", count,id, loop );
      loop--;
      count++;
      flag = (id + 1) % 2;
      //green_cond_signal(&cond);
      green_mutex_unlock(&mutex);
  //  } else {
    //  printf("tread %d waiting now \n", id);
      //green_cond_wait(&cond);

    }
  }
//  green_mutex_unlock(&mutex);
}*/

int main(){

  green_mutex_init(&mutex);
  //printf("mutix is initiated\n" );
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
