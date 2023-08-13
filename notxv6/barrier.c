#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <assert.h>
#include <pthread.h>

static int nthread = 1;
static int round = 0;

struct barrier {
  pthread_mutex_t barrier_mutex;
  pthread_cond_t barrier_cond;
  int nthread;      // Number of threads that have reached this round of the barrier
  int round;     // Barrier round
} bstate;

static void
barrier_init(void)
{
  assert(pthread_mutex_init(&bstate.barrier_mutex, NULL) == 0);
  assert(pthread_cond_init(&bstate.barrier_cond, NULL) == 0);
  bstate.nthread = 0;
}

static void 
barrier()
{
  // YOUR CODE HERE
  //
  // Block until all threads have called barrier() and
  // then increment bstate.round.
  //
  // 锁定屏障互斥锁，以确保对屏障状态的独占访问
  pthread_mutex_lock(&bstate.barrier_mutex);
  
  // 如果尚未所有线程都到达屏障
  if (++bstate.nthread < nthread) {
      // 当前线程等待在屏障条件变量上，并在等待期间释放屏障互斥锁
      pthread_cond_wait(&bstate.barrier_cond, &bstate.barrier_mutex);
  } else {
      // 当最后一个线程到达屏障
      // 重置已到达屏障的线程计数为 0
      bstate.nthread = 0;
      
      // 增加轮次计数以表示进入了新的屏障同步轮次
      bstate.round++;
      
      // 通过广播在屏障条件变量上唤醒所有等待的线程，以便它们可以继续执行
      pthread_cond_broadcast(&bstate.barrier_cond);
  }
  
  // 解锁屏障互斥锁，允许其他线程继续访问屏障并调用 barrier() 函数
  pthread_mutex_unlock(&bstate.barrier_mutex);

}

static void *
thread(void *xa)
{
  long n = (long) xa;
  long delay;
  int i;

  for (i = 0; i < 20000; i++) {
    int t = bstate.round;
    assert (i == t);
    barrier();
    usleep(random() % 100);
  }

  return 0;
}

int
main(int argc, char *argv[])
{
  pthread_t *tha;
  void *value;
  long i;
  double t1, t0;

  if (argc < 2) {
    fprintf(stderr, "%s: %s nthread\n", argv[0], argv[0]);
    exit(-1);
  }
  nthread = atoi(argv[1]);
  tha = malloc(sizeof(pthread_t) * nthread);
  srandom(0);

  barrier_init();

  for(i = 0; i < nthread; i++) {
    assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
  }
  for(i = 0; i < nthread; i++) {
    assert(pthread_join(tha[i], &value) == 0);
  }
  printf("OK; passed\n");
}
