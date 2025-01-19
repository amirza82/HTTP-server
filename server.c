#include "head.h"

SA_IN sockaddr;
int sockfd, connfd;
shobj *so; //shared object
bool ischild = 0;
FILE *fp;

int main(void)
{
    // signal to quit
    signal(SIGINT, sighandle);

    // make the socket file descriptor
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        DIE("socket creation faild:");
    // make reusable
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &(int){1}, sizeof(int)) == -1)
        DIE("faild to set sock options:");

    // set address
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(SERVER_PORT);
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    socklen_t sockaddr_len = sizeof(sockaddr);

    // bind & listen
    if ((bind(sockfd, (SA *) &sockaddr, sizeof(sockaddr))) == -1)
        DIE("bind faild:");
    if ((listen(sockfd, BACKLOG)) == -1)
        DIE("listen faild:");

    // create shared memory
    so = mmap(NULL, sizeof(shobj), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    // initialize shared memory
    so->proccount = 0;
    if (sem_init(&so->proclock, 1, 1) == -1)
        DIE("sem_init faild:");
    if (sem_init(&so->loglock, 1, 1) == -1)
        DIE("sem_init faild:");

    // accept connections
    while (1) {
        if ((connfd = accept(sockfd, (SA *) &sockaddr, &sockaddr_len)) == -1)
            DIE("accept faild:");
        // we have a open connectio
        int pid;
        sem_wait(&so->proclock);
        if (so->proccount >= MAX_CONN_HANDLE && so->proccount != 0) {
            printf("so->proccount: %d\n", so->proccount);
            fflush(stdout);
            sem_post(&so->proclock);
            wait(NULL); // if limit is reached, wait
        } else
            sem_post(&so->proclock);

        if ((pid = fork()) == -1)
            DIE_GRACE("fork faild:");
        else if (pid == 0) { // we are in child process
            int proc_ext_status;
            ischild = 1;

            proc_ext_status = handle_conn(connfd);

            printf("proc %d exit: %d\n", getpid(), proc_ext_status);
            fflush(stdout);
            exit(proc_ext_status);
        }
        close(connfd);
    }

    return 0;
}
