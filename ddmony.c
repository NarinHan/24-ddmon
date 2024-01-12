#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <execinfo.h>

#ifdef DEBUG
  #define debug(fn) fn
#else
  #define debug(fn)
#endif

pthread_mutex_t channel_lock = PTHREAD_MUTEX_INITIALIZER;

int pthread_mutex_lock(pthread_mutex_t * lock){
  int fd = open("channel", O_WRONLY | O_SYNC);
  if(fd == -1){
    printf("Error opening file: %s\n", strerror(errno));
    return 1;
  }
  int (*pthread_mutex_lock_cp)(pthread_mutex_t*);
  int (*pthread_mutex_unlock_cp)(pthread_mutex_t*);
  char * error;
  int return_value;

  pthread_mutex_lock_cp = dlsym(RTLD_NEXT, "pthread_mutex_lock");
  if((error = dlerror()) != 0x0) exit(EXIT_FAILURE);
  pthread_mutex_unlock_cp = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
  if((error = dlerror()) != 0x0) exit(EXIT_FAILURE);

  return_value = pthread_mutex_lock_cp(lock);
  pthread_t pid = pthread_self();
  
  pthread_mutex_lock_cp(&channel_lock);
  write(fd, &pid, sizeof(pid));
  write(fd, &lock, sizeof(lock));
  pthread_mutex_unlock_cp(&channel_lock);

  close(fd);

  void * arr[10] ;
  char ** stack ;

  printf("pthread_lock(%lu)=%p\n", pid, lock) ;

  size_t sz = backtrace(arr, 10) ;
  stack = backtrace_symbols(arr, sz) ;

  printf("Stack trace\n") ;
  printf("============\n") ;
  for (int i = 0 ; i < sz ; i++)
          printf("[%d] %s\n", i, stack[i]) ;
  printf("============\n\n") ;
  debug(printf("Yaho1\n"););
  return return_value;
}


int pthread_mutex_unlock(pthread_mutex_t * lock){
  int fd = open("channel", O_WRONLY | O_SYNC);
  if(fd == -1){
    printf("Error opening file: %s\n", strerror(errno));
    return 1;
  }
  int (*pthread_mutex_lock_cp)(pthread_mutex_t*);
  int (*pthread_mutex_unlock_cp)(pthread_mutex_t*);
  char * error;
  int return_value;

  pthread_mutex_lock_cp = dlsym(RTLD_NEXT, "pthread_mutex_lock");
  if((error = dlerror()) != 0x0) exit(EXIT_FAILURE);
  pthread_mutex_unlock_cp = dlsym(RTLD_NEXT, "pthread_mutex_unlock");
  if((error = dlerror()) != 0x0) exit(EXIT_FAILURE);

  return_value = pthread_mutex_unlock_cp(lock);

  pthread_t pid = pthread_self();

  pthread_mutex_lock_cp(&channel_lock);
  write(fd, &pid, sizeof(pid));
  write(fd, &lock, sizeof(lock));
  pthread_mutex_unlock_cp(&channel_lock);

  close(fd);

  void * arr[10] ;
  char ** stack ;

  printf("pthread_unlock(%lu)=%p\n", pid, lock) ;

  size_t sz = backtrace(arr, 10) ;
  stack = backtrace_symbols(arr, sz) ;

  printf("Stack trace\n") ;
  printf("============\n") ;
  for (int i = 0 ; i < sz ; i++)
          printf("[%d] %s\n", i, stack[i]) ;
  printf("============\n\n") ;
  debug(printf("Yaho2\n"););
  return return_value;
}