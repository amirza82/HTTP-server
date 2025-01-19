#include "head.h"

// sighandle: close connections and exit;
void sighandle(int sig)
{
    extern int sockfd;
    extern bool ischild;
    if (sig != SIGINT)
        return; // for now, we only handle SIGINT
    if (ischild)
        return;
    // it's the master process
    while (wait(NULL) != -1) // until there is child left, wait for them to complete
        ;
    close(sockfd);           // then close the main socket and exit
    exit(sig);
}

// bintohex: convert bin to hex removing useless characters
char *bintohex(char *input, size_t len)
{
    char *result;
    char *hexits = "0123456789ABCDEF";
    size_t i;
    if (input == NULL || len == 0)
        return NULL;

    result = calloc((len*3)+1, sizeof (char));

    for (i = 0; i < len; ++i) {
        result[(i*3)+0] = hexits[input[i] >> 4];
        result[(i*3)+1] = hexits[input[i] & 0x0F];
        result[(i*3)+2] = ' ';
    }
    return (char *) result;
}

// httpreqtoabspath: parse http request and return abs path
char *httpreqtoabspath(char *req, char *abspath)
{
    int startpath, endpath;

    // find the start
    if (*req == 'G')
        startpath = 4;
    else if (*req == 'P')
        startpath = 5;
    else
        return NULL;

    // find the end
    for (endpath = 0; ; ++endpath) {
        if (req[endpath] == '\r' && req[endpath+1] == '\n')
            break;
    }
    endpath -= 8;
    if (strncmp(req+endpath, "HTTP", (size_t) 4) == 0) // check if protocol is http //TODO: also check the version is 1.0
        endpath -= 2;
    else
        return NULL;

    memset(abspath, 0, PATH_MAX);
    strncpy(abspath, req+startpath, endpath-startpath+1);
    return abspath;
}

// handle_conn: read, respond to, and close the connection
int handle_conn(int connfd)
{
                            /*NOTE:
                             * I assumed all read and write functions
                             * return the full requested length
                             * or -1 on error. if the full length is 
                             * not returned, then we assume this is the
                             * end of message and break from the loop*/

                            //TODO: fix the mess described above
    char recvline[MAXLINE];
    char fullline[MAXLINE];
    char sendline[MAXLINE];
    char rpath   [PATH_MAX];
    memset(recvline, 0, sizeof(recvline)); // just
    memset(fullline, 0, sizeof(fullline)); // to
    memset(sendline, 0, sizeof(sendline)); // be
    memset(rpath   , 0, sizeof(   rpath)); // safe :)
    int n;
    int seek;
    int write_bytes, read_bytes;
    FILE *filep = NULL;
    httpinfo hi;
    extern shobj *so;

    //read
    // while ((n = read(connfd, recvline, sizeof (recvline)-1)) > 0) {
    //     len += n;
    //     if (len < MAXLINE-1) // sub by 1 because strncat null terminates destination str
    //         ncat(fullline, recvline, (size_t) n);
    //     else
    //         break;
    //     if (strcmp((fullline+len-4), "\r\n\r\n") == 0)
    //         break;
    // }
    // if (n < 0)
    //     DIE("read error");
    //TODO: use pool to set timeout
    n = read(connfd, recvline, sizeof recvline);
    if (n == -1)
        INVALID_REQ("read faild:");
    // parse http req
        // 1.get method
    if (strncmp(recvline, "GET", (size_t) 3) == 0)
        hi.reqtype = GET, seek = 3;
    else if (strncmp(recvline, "POST", (size_t) 4) == 0)
        hi.reqtype = POST, seek = 4;
    else
        INVALID_REQ("unknown method. only GET and POST are supported!");
    if (hi.reqtype == POST) {
        sem_wait(&so->proclock);
        so->proccount++;
        sem_post(&so->proclock);
    }
        // 2.get URI
    while (strncmp(recvline+seek, "\r\n", (size_t) 2) != 0)
        seek++;
    while (strncmp(recvline+seek, " HTTP/", 6) != 0)
        if (seek < MAXLINE)
            seek--;
        else
            INVALID_REQ("invalid http format");
    if (hi.reqtype == GET)
        memcpy(hi.URI, recvline+4, (seek-4+1 < MAXURI) ? seek-4+1 : 0), hi.URI[seek-4] = '\0'; // null terminated URI
    else // POST
        memcpy(hi.URI, recvline+5, (seek-5+1 < MAXURI) ? seek-5+1 : 0), hi.URI[seek-5] = '\0'; // null terminated URI

        // 3.get file size
    if (hi.reqtype == POST) {
        while (strncmp(recvline+seek, "\r\nContent-Length: ", (size_t) 18) != 0)
            if (strncmp(recvline+seek, "\r\n\r\n", 4) == 0)
                INVALID_REQ("\"Content-Length\" is required");
            else
                seek++;
    }
    seek += 18;
    hi.filesize = atoi(recvline+seek);

        // 4. if method is post, go to end of header
    if (hi.reqtype == POST) {
        while (seek < MAXLINE && strncmp(recvline+seek, "\r\n\r\n", 4) != 0)
            seek++;
        seek+=4; // now we are pointing to the first byte of data
        if (seek+4 >= MAXLINE)
            INVALID_REQ("header must be less than 4096 bytes");
    }

    // access log file
    sem_wait(&so->loglock);
    FILE *logfp = fopen("log.txt", "a");
    if (logfp == NULL)
        INVALID_REQ("faild to open log file:");
    time_t now = time(NULL);
    if (now == -1)
        INVALID_REQ("getting time faild:");
    fprintf(logfp, "%ld req: %s path: %s\n", now, (hi.reqtype == GET) ? "GET" : "POST", hi.URI);
    fclose(logfp);
    sem_post(&so->loglock);

    switch (hi.reqtype) {
        case GET:
            if ((filep = fopen(hi.URI, "r")) == NULL) {
                fprintf(stderr, "%s:%d:%s\n", __FILE_NAME__, __LINE__, strerror(errno));
                SEND404(connfd);
                close(connfd);
                return -1;
            }
            memcpy(sendline, "HTTP/1.0 200 OK\r\n\r\n", 19);
            read_bytes = 19;
            while (read_bytes) {
                write_bytes = 0;
                while (write_bytes != read_bytes) //TODO: check for write error
                    write_bytes += write(connfd, sendline, read_bytes);
                read_bytes = fread(sendline, sizeof (char), MAXLINE, filep); //TODO: check for read error
            }
            break;

        case POST:
            //TODO: lock the file first
            if ((filep = fopen(hi.URI, "w+")) == NULL) {
                close(connfd);
                fprintf(stderr, "%s:%d:%s\n", __FILE_NAME__, __LINE__, strerror(errno));
                return -1;
            }
            hi.filesize -= fwrite(recvline+seek, sizeof (char), (hi.filesize <= MAXLINE-seek) ? hi.filesize : MAXLINE-seek, filep);
            if (hi.filesize == -1)
                INVALID_REQ("filad to write:");
            while (hi.filesize) {
                if ((read_bytes = read(connfd, recvline, sizeof (recvline))) == -1)
                    INVALID_REQ("faild to read from socket:");
                if ((write_bytes = fwrite(recvline, sizeof (char), read_bytes, filep)) == -1)
                    INVALID_REQ("faild to write:");
                if (read_bytes != write_bytes)
                    INVALID_REQ("faild to write:");
                else
                    hi.filesize -= write_bytes;
            }
            break;
        default:
            break;
    }


    //close
    fclose(filep);
    close(connfd);
    if (hi.reqtype == POST) {
        sem_wait(&so->proclock);
        so->proccount--;
        sem_post(&so->proclock);
    }
    return 0;
}

// ncat: concatenate n bytes of src to dest + null terminate
char *ncat(char *dest, char*src, size_t n)
{
    size_t len = strlen(dest);
    memcpy(dest+len, src, n);
    *(dest+len+n) = '\0';
    return dest;
}
