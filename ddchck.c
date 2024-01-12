#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <pthread.h>

#define MAX_NUM 10

typedef enum {
    lock,
    unlock
} flag ;

typedef struct mymutex_t {
    pthread_mutex_t *addr ;
    char line_addr[16] ;
    int thread_index ;
    struct mymutex_t *next ;
} mymutex_t ;

typedef struct mythread_t {
    pthread_t thread_id ;
    int thread_mutex[MAX_NUM] ; // thread acquired mutex or not
} mythread_t ;

int graph[MAX_NUM][MAX_NUM] ;
mymutex_t *mymutex[MAX_NUM] ;
mythread_t mythread[MAX_NUM] ;
pthread_mutex_t *mutex_dic[MAX_NUM] ; // mutex index & address dictionary

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
check_and_add_thread(pthread_t pthread_id)
{
    for (int i = 0; i < MAX_NUM; i++) {
        if (mythread[i].thread_id == 0) {
            mythread[i].thread_id = pthread_id ;
            return i ;
        } else if (mythread[i].thread_id == pthread_id) {
            return i ;
        }
    }
    return -1 ;
}

int
find_mutex_index(pthread_mutex_t *mtx_addr) 
{
    for (int i = 0; i < MAX_NUM; i++) {
        if (mutex_dic[i] == mtx_addr)
            return i ;
    }
    return -1 ;
}

void
add_or_create_mutex(pthread_mutex_t *mtx_addr, char *line_addr, int thread_idx)  
{
    int idx = find_mutex_index(mtx_addr) ; // chceck the mutex dictionary first    
    if (idx == -1) { // first mutex
        for (int i = 0; i < MAX_NUM; i++) {
            if (mutex_dic[i] == NULL) {
                mymutex[i] = (mymutex_t *)malloc(sizeof(mymutex_t)) ;  
                mymutex[i]->addr = mtx_addr ;
                strncpy(mymutex[i]->line_addr, line_addr, strlen(line_addr)) ;
                mymutex[i]->line_addr[strlen(line_addr)] = '\0' ;
                mymutex[i]->thread_index = thread_idx ;
                mymutex[i]->next = NULL ;
                
                mutex_dic[i] = mtx_addr ;
                break ;
            }
        }
    } else { // existing mutex
        mymutex_t *current = mymutex[idx] ;
        while (current->next != NULL) {
            current = current->next ; // traverse to the end of the list
        }
        current->next = (mymutex_t *)malloc(sizeof(mymutex_t)) ;
        current->next->addr = mtx_addr ;
        strncpy(current->next->line_addr, line_addr, strlen(line_addr)) ;
        current->next->line_addr[strlen(line_addr)] = '\0' ;
        current->next->thread_index = thread_idx ;
        current->next->next = NULL ;
    }
}

void 
update_graph(int mutex_idx, int flag) 
{
    
}

int
main(int argc, char *argv[])
{
    if (mkfifo("channel", 0666)) {
        if (errno != EEXIST) {
            perror("fail to open fifo : ") ;
            exit(EXIT_FAILURE) ;
        }
    }

    int fifo_fd = open("channel", O_RDONLY | O_SYNC) ;
    
    int read_len ;
    pthread_t pthread_id ;
    pthread_mutex_t *mtx_addr ;
    int line_addr_sz ;
    char *line_addr ;

    printf("Thread ID | Mutex Address | Line Address\n") ;
    while (1) {
        if (read_bytes(fifo_fd, &pthread_id, sizeof(pthread_id)) == 1) { // thread id
            perror("reading thread id error : ") ;
            exit(EXIT_FAILURE) ;
        }
        printf("%lu : ", pthread_id) ;

        if (read_bytes(fifo_fd, &mtx_addr, sizeof(mtx_addr)) == 1) { // mutex address
            perror("reading mutex address error : ") ;
            exit(EXIT_FAILURE) ;
        }
        printf("%p : ", mtx_addr) ;

        if (read_bytes(fifo_fd, &line_addr_sz, sizeof(line_addr_sz)) == 1) { // line address size
            perror("reading line address size error : ") ;
            exit(EXIT_FAILURE) ; 
        }

        line_addr = (char *)malloc(line_addr_sz) ;
        if (read_bytes(fifo_fd, line_addr, line_addr_sz) == 1) { // line address
            perror("reading line address error : ") ;
            exit(EXIT_FAILURE) ;
        }
        printf("%s\n", line_addr) ;

        // thread 있는지 확인 : 없으면 만들어서 넣기
        int thread_idx = check_and_add_thread(pthread_id) ;

        // Save the data into data structure
        add_or_create_mutex(mtx_addr, line_addr, thread_idx) ;

        // For checking
        printf("Let's print\n") ;
        for (int i = 0; i < MAX_NUM; i++) {
            if (mymutex[i] != NULL) {
                printf("mymutex[%d] has mutex address %p\n", i, mymutex[i]->addr) ;
            }
        }

        for (int i = 0; i < MAX_NUM; i++) {
            if (mymutex[i]->addr == mtx_addr) {
                if (mythread[mymutex[i]->thread_index].thread_mutex[i] == 0) {
                    update_graph(i, lock) ;
                }
            }
        }

        // addr2line
        char cmd[512] ;
        snprintf(cmd, sizeof(cmd), "addr2line -e target %s", line_addr);

        FILE *fp = popen(cmd, "r") ;
        if (fp == NULL) {
            perror("popen error : ") ;
            exit(EXIT_FAILURE) ;
        }

        char line[512] ;
        if (!(fgets(line, sizeof(line) - 1, fp))) {
            char *filename = strtok(line, ":") ; // extract file name
            char *linenum = strtok(NULL, ":") ; // extract line number
            if (linenum != NULL) {
                printf("%s", linenum) ;
            }

            // TODO: line number 저장
        }

        pclose(fp) ;

        // algorithm

        free(line_addr) ;
    }

    close(fifo_fd) ;

    return 0 ;
}
