#ifndef HEAD_H
#define HEAD_H

#include <time.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <asm-generic/socket.h>
#include <sys/sendfile.h>
#include <fcntl.h>
#include <arpa/inet.h>
#include <assert.h>
#include <linux/limits.h>
#include <signal.h>
#include <stdbool.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <sys/types.h>
#include <unistd.h>
#include <wait.h>
#include <limits.h>
#include <sys/file.h>

#define MAX_CONN_HANDLE 5 // for each connection, a new process is made
#define MAXLINE 4096 // max size of buffers
#define POST_LIMIT 5
#define  GET_LIMIT 0
#define MAXURI 2000
#define SA struct sockaddr
#define SA_IN struct sockaddr_in

enum req_type {
    GET,
    POST
};

typedef struct httpinfo {
    enum req_type reqtype;
    char URI[MAXURI];
    int64_t filesize; // size of data after \r\n\r\n
} httpinfo;

// Handle errors
#define DIE(MSG) do { fprintf(stderr, "%s:%d: Died:%s error: %s\n", __FILE_NAME__, __LINE__,\
                              MSG, strerror(errno));                                        \
                              fflush(stderr);                                               \
                              exit(errno);                                                  \
                              } while (0)

#define DIE_GRACE(MSG) do { fprintf(stderr, "%s:%d: Died:%s error: %s\n", __FILE_NAME__, __LINE__,\
                                    MSG, strerror(errno));                                        \
                                    fflush(stderr);                                               \
                                    kill(getpid(), SIGINT);                                       \
                                    } while (0)

#define INVALID_REQ(MSG) do { fprintf(stderr, "%s:%d: Died:%s error: %s\n", __FILE_NAME__, __LINE__,\
                                      MSG, strerror(errno));                                        \
                                      fflush(stderr);                                               \
                                      close(connfd);                                                \
                                      if (filep) fclose(filep);                                     \
                                      exit(-1);                                                     \
                                      } while (0) //TODO: complete this later

#define SEND404(FD) do { write(FD, "HTTP/1.0 404 Not found\r\n\r\n", strlen("HTTP/1.0 404 Not found\r\n\r\n")); } while (0)
// utilities
char *ncat(char *dest, char*src, size_t n); // maybe it's not a good idea...
char *bintohex(char *input, size_t len);

/*        SERVER SPECIFIC        */

#define SERVER_PORT 8080
#define BACKLOG 4 // for function 'listen'

typedef struct shobj {
    int proccount;      // number of processes that are running right now
    sem_t proclock;     // lock must be used in order to change the proccount vlue
    sem_t loglock;      // lock writing to the log file
} shobj;

char *httpreqtoabspath(char *req, char *abspath);

int handle_conn(int connfd);


/*        CLIENT SPECIFIC        */

#define CLIENT_PORT 80

void sighandle(int sig);
#endif


//TODO: 1. implement logging: the incomming requests and answer to them must be logged
//TODO: 2. implement router:  master process recv reqs and distribiuts them between slaves (round robin)
//TODO: 3. implement threads: each request must be handled by a separate thread. detach them with pthread_detach()
//TODO: 4. implement mode fn: a function for each mode. POST and GET are handled saparatly and also another function to detect the mode
