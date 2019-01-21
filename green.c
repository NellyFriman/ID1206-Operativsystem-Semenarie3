#include <stdlib.h>
#include <stdio.h>
#include <ucontext.h>
#include <assert.h>
#include <signal.h>
#include <sys/time.h>
#include "green.h"

#define FALSE 0
#define TRUE 1

#define PERIOD 100
#define STACK_SIZE 4096

static sigset_t block;

static ucontext_t main_cntx = {0};
static green_t main_green = {&main_cntx, NULL, NULL, NULL, NULL, FALSE};

static green_t *running = &main_green;
struct green_t *ready_queue = NULL;

void timer_handler(int);

static void init() __attribute__((constructor));

void init() {

  sigemptyset(&block);
  sigaddset(&block, SIGVTALRM);

  struct sigaction act = {0};
  struct timeval interval;
  struct itimerval period;

  act.sa_handler = timer_handler;
  assert(sigaction(SIGVTALRM, &act, NULL) == 0);

  interval.tv_sec = 0;
  interval.tv_usec = PERIOD;
  period.it_interval = interval;
  period.it_value = interval;
  setitimer(ITIMER_VIRTUAL, &period, NULL);

  getcontext(&main_cntx);
}

void add (green_t *new, green_t **list){

  if(*list == NULL){
  *list = new;
  } else {
    green_t *tmp = *list;
    while (tmp->next != NULL){
      tmp = tmp->next;
    }
    tmp->next = new;
  }
}

green_t *select_next(green_t **list){

  if (*list == NULL) {
    return &main_green;
  }
  green_t *thread = *list;
  *list = thread->next;
  thread->next = NULL;
  return thread;
}
void timer_handler(int sig){

  green_t *susp = running;

  add(susp, &ready_queue);
  green_t *next = select_next(&ready_queue);

  running = next;
  swapcontext(susp->context, next->context);
}

void green_thread(){
  green_t *this = running;

  (*this->fun)(this->arg);

  //place waiting (joining) thread in ready queue
  if(this->join != NULL){
    add(this->join, &ready_queue);
  }
  //free alocated memory sturctures
  free(this->context->uc_stack.ss_sp);
  free((void*)this->context);
  // we're zombie
  this->zombie = TRUE;
  //find the next thread to running
  green_t *next = select_next(&ready_queue);
//  printf("timer count %d\n",timer_count );
  running = next;
  setcontext(next->context);
}

int green_create(green_t *new, void *(*fun)(void*), void *arg){

  ucontext_t *cntx = (ucontext_t *)malloc(sizeof(ucontext_t));
  getcontext(cntx);

  void *stack = malloc(STACK_SIZE);

  cntx->uc_stack.ss_sp = stack;
  cntx->uc_stack.ss_size = STACK_SIZE;

  makecontext(cntx, green_thread, 0);
  new->context = cntx;
  new->fun = fun;
  new->arg = arg;
  new->next = NULL;
  new->join = NULL;
  new->zombie = FALSE;
  // add new to the ready queue
  add(new, &ready_queue);

  return 0;
}

int green_yield(){
  sigprocmask(SIG_BLOCK, &block, NULL);

  green_t *susp = running;
  // add susp to ready queue
  add(susp, &ready_queue);
  //select the next thread to execute
  green_t *next = select_next(&ready_queue);

  running = next;

  swapcontext(susp->context, next->context);
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

int green_join(green_t *thread){
  sigprocmask(SIG_BLOCK, &block, NULL);

  if(thread->zombie){
    return 0;
  }
  green_t *susp = running;
  add(susp, &thread->join);

  green_t *next = select_next(&ready_queue);
  running = next;
  swapcontext(susp->context, next->context);
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

void green_cond_init(green_cond_t* cond){
  cond->queue = NULL;
}
/********************************************************
Other version of green_cond_wait with just one argument

void green_cond_wait(green_cond_t* cond){

  green_t *susp = running;

  add(susp, &cond->queue);
  green_t *next = select_next(&ready_queue);

  running = next;
  swapcontext(susp->context, next->context);
}
***********************************************************/
int green_cond_wait(green_cond_t *cond, green_mutex_t *mutex){
  sigprocmask(SIG_BLOCK, &block, NULL);

  green_t *susp = running;

  add(susp, &cond->queue);

  if(mutex != NULL){
    if (mutex->taken)
      mutex->taken = FALSE;
      add(mutex->susp, &ready_queue);
  }
  green_t *next = select_next(&ready_queue);
  running = next;
  swapcontext(susp->context, next->context);

  if(mutex != NULL){

    while(mutex->taken) {

      add(susp, &mutex->susp);

      green_t *next = select_next(&ready_queue);
      running = next;
      swapcontext(susp->context, next->context);
    }
    mutex->taken = TRUE;
    sigprocmask(SIG_UNBLOCK, &block, NULL);
    }

  return 0;
}

void green_cond_signal(green_cond_t *cond){

  if (cond->queue == NULL) {
    return;
  }
  green_t *thread = select_next(&cond->queue);
  add(thread, &ready_queue);
}

int green_mutex_init(green_mutex_t *mutex){
  mutex->taken = FALSE;
  mutex->susp = NULL;
}

int green_mutex_lock(green_mutex_t *mutex){
  sigprocmask(SIG_BLOCK, &block, NULL);

  green_t *susp = running;
  while(mutex->taken == TRUE){

    add(susp, &mutex->susp);

    green_t *next = select_next(&ready_queue);
    running = next;
    swapcontext(susp->context, next->context);
  }
  mutex->taken = TRUE;
  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}

int green_mutex_unlock(green_mutex_t *mutex){
  sigprocmask(SIG_BLOCK, &block, NULL);

  if(mutex->susp != NULL)
  add(mutex->susp, &ready_queue);

  mutex->susp = NULL;
  mutex->taken = FALSE;

  sigprocmask(SIG_UNBLOCK, &block, NULL);
  return 0;
}
