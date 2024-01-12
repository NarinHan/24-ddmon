#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <dlfcn.h>
#include <execinfo.h>

static int (*lockcp)(pthread_mutex_t *) ;
static int (*unlockcp)(pthread_mutex_t *) ;

pthread_mutex_t writing_mtx = PTHREAD_MUTEX_INITIALIZER ;

int 
write_bytes(int fd, void *writing_buf, size_t len)
{
    // return 0 if all given bytes are succesfully sent
    // return 1 otherwise

    char *p = (char *)writing_buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t written ;
        if ((written = write(fd, p, len - acc)) == -1) 
            return 1 ;
        p += written ;
        acc += written ;
    }

    return 0 ;
}

int
read_bytes(int fd, void *reading_buf, size_t len)
{
    // return 0 if all given bytes are succesfully read
    // return 1 otherwise

    char *p = (char *)reading_buf ;
    size_t acc = 0 ;

    while (acc < len) {
        size_t received ;
        if ((received = read(fd, p, len - acc)) == -1)
            return 1 ;
        p += received ;
        acc += received ;
    }

    return 0 ;
}

int
pthread_mutex_lock(pthread_mutex_t *mtx)
{
	char *error ;

	int fifo_fd = open("channel", O_WRONLY | O_SYNC) ;

	lockcp = dlsym(RTLD_NEXT, "pthread_mutex_lock") ; // real pthread_mutex_lock
  	unlockcp = dlsym(RTLD_NEXT, "pthread_mutex_unlock") ;
	if ((error = dlerror()) != 0x0)
		exit(EXIT_FAILURE) ;

	// backtrace
	void *arr[10] ;
	char **stack ;

	size_t trace_size = backtrace(arr, 10) ;
	stack = backtrace_symbols(arr, trace_size) ;

	fprintf(stdout, "====================\n") ;
	fprintf(stdout, "Stack trace [lock]\n") ;
	fprintf(stdout, "====================\n") ;
	for (int i = 0; i < trace_size; i++) {
		fprintf(stdout, "[%d] %s\n", i, stack[i]) ;
	}
	fprintf(stdout, "====================\n") ;

	// extract address from the result of the backtrace
	char *p = stack[1] ;
	while (*p != '+' && *p) {
		p++ ;
	}
	if (*p == '+') {
		p++ ;
	}
	char *line_addr_str = p ;
	while (*p != ')' && *p) {
		p++ ;
	}
	*p = '\0' ;

	// Convert and compute the address
	intptr_t line_addr = strtoul(line_addr_str, NULL, 0);
	line_addr -= 4;
	sprintf(line_addr_str, "%p", (void *)line_addr) ;
	int line_addr_sz = strlen(line_addr_str) ;

	// fprintf(stdout, "lock... ") ;
	// fprintf(stdout, "thread id : %ld / mtx addr : %p / line addr : %p\n", pthread_self(), mtx, (void *)line_addr) ;
 
	printf("lock...\n") ;
	fprintf(stdout, "thread id : %ld\n", pthread_self()) ;
	fprintf(stdout, "mtx addr  : %p\n", mtx) ;
	fprintf(stdout, "line addr : %s\n", line_addr_str) ;
	
	// Write on FIFO
	pthread_t pthread_id = pthread_self() ;
  	lockcp(&writing_mtx) ;
	if (write_bytes(fifo_fd, &pthread_id, sizeof(pthread_id)) == 1) { // thread id
		perror("writing thread id error : ") ;
		exit(EXIT_FAILURE) ;
	}
	if (write_bytes(fifo_fd, &mtx, sizeof(mtx)) == 1) { // mutex address
		perror("writing mutex address error : ") ;
		exit(EXIT_FAILURE) ;
	}
	if (write_bytes(fifo_fd, &line_addr_sz, sizeof(line_addr_sz)) == 1) { // line address size
		perror("writing line address size error : ") ;
		exit(EXIT_FAILURE) ;	
	}
	if (write_bytes(fifo_fd, line_addr_str, line_addr_sz) == 1) { // line address
		perror("writing line address error : ") ;
		exit(EXIT_FAILURE) ;
	}
  	unlockcp(&writing_mtx) ;

	int ret = lockcp(mtx) ;

	close(fifo_fd) ;

	return ret ;
}

int
pthread_mutex_unlock(pthread_mutex_t *mtx)  
{
	char *error ;

	int fifo_fd = open("channel", O_WRONLY | O_SYNC) ;

	unlockcp = dlsym(RTLD_NEXT, "pthread_mutex_unlock") ; // real pthread_mutex_unlock
	if ((error = dlerror()) != 0x0)
		exit(EXIT_FAILURE) ;

	int ret = unlockcp(mtx) ;
  	
	// backtrace
	void *arr[10] ;
	char **stack ;

	size_t sz = backtrace(arr, 10) ;
	stack = backtrace_symbols(arr, sz) ;

	fprintf(stdout, "====================\n") ;
	fprintf(stdout, "Stack trace [unlock]\n") ;
	fprintf(stdout, "====================\n") ;
	for (int i = 0; i < sz; i++) {
		fprintf(stdout, "[%d] %s\n", i, stack[i]) ;
	}
	fprintf(stdout, "====================\n") ;

	// extract address from the result of the backtrace
	char *p = stack[1] ;
	while (*p != '+' && *p) {
		p++ ;
	}
	if (*p == '+') {
		p++ ;
	}
	char *line_addr_str = p ;
	while (*p != ')' && *p) {
		p++ ;
	}
	*p = '\0' ;

	// Convert and compute the address
	intptr_t line_addr = strtoul(line_addr_str, NULL, 0);
	line_addr -= 4;
	sprintf(line_addr_str, "%p", (void *)line_addr) ;
	int line_addr_sz = strlen(line_addr_str) ;

	fprintf(stdout, "unlock...\n") ;
	fprintf(stdout, "thread id : %ld\n", pthread_self()) ;
	fprintf(stdout, "mtx addr  : %p\n", mtx) ;
	fprintf(stdout, "line addr : %s\n", line_addr_str) ;

	// Write on FIFO
	pthread_t pthread_id = pthread_self() ;
  	lockcp(&writing_mtx) ;
	if (write_bytes(fifo_fd, &pthread_id, sizeof(pthread_id)) == 1) { // thread id
		perror("writing thread id error : ") ;
		exit(EXIT_FAILURE) ;
	}
	if (write_bytes(fifo_fd, &mtx, sizeof(mtx)) == 1) { // mutex address
		perror("writing mutex address error : ") ;
		exit(EXIT_FAILURE) ;
	}
	if (write_bytes(fifo_fd, &line_addr_sz, sizeof(line_addr_sz)) == 1) { // line address size
		perror("writing line address size error : ") ;
		exit(EXIT_FAILURE) ;	
	}
	if (write_bytes(fifo_fd, line_addr_str, line_addr_sz) == 1) { // line address
		perror("writing line address error : ") ;
		exit(EXIT_FAILURE) ;
	}
  	unlockcp(&writing_mtx) ;

	close(fifo_fd) ;

	return ret ;
}