#include "httpd.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>

#define CONNMAX 1000
static int listenfd, clients[CONNMAX], clientfd;

// header storage
typedef struct { char *name, *value; } header_t;
static header_t reqhdr[17] = { {"\0", "\0"} };

// globals
char *method, *uri, *qs, *prot;
char *payload;
int   payload_size;

static void startServer(const char *port);
static void respond(int);

// entry point
void serve_forever(const char *PORT) {
    struct sockaddr_in clientaddr;
    socklen_t addrlen;
    int slot = 0;

    printf("Server started %shttp://127.0.0.1:%s%s\n",
           "\033[92m", PORT, "\033[0m");

    for (int i = 0; i < CONNMAX; i++) clients[i] = -1;
    startServer(PORT);
    signal(SIGCHLD, SIG_IGN);

    while (1) {
        addrlen = sizeof(clientaddr);
        clients[slot] = accept(listenfd,
                               (struct sockaddr *)&clientaddr,
                               &addrlen);
        if (clients[slot] < 0) {
            perror("accept() error");
        } else {
            if (fork()==0) {
                respond(slot);
                exit(0);
            }
        }
        while (clients[slot] != -1)
            slot = (slot+1) % CONNMAX;
    }
}

static void startServer(const char *port) {
    struct addrinfo hints, *res, *p;
    memset(&hints,0,sizeof(hints));
    hints.ai_family   = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags    = AI_PASSIVE;
    if (getaddrinfo(NULL, port, &hints, &res)!=0) {
        perror("getaddrinfo() error");
        exit(1);
    }
    for (p=res; p; p=p->ai_next) {
        int opt=1;
        listenfd = socket(p->ai_family,p->ai_socktype,0);
        setsockopt(listenfd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
        if (listenfd==-1) continue;
        if (bind(listenfd,p->ai_addr,p->ai_addrlen)==0) break;
    }
    if (!p) {
        perror("socket() or bind()");
        exit(1);
    }
    freeaddrinfo(res);
    if (listen(listenfd,1000000)!=0) {
        perror("listen() error");
        exit(1);
    }
}

static char *buf;
static void respond(int n) {
    buf = malloc(65535);
    int rcvd = recv(clients[n], buf, 65535, 0);
    if (rcvd>0)       analyze_http(buf, rcvd);
    // reroute stdout to socket
    clientfd = clients[n];
    dup2(clientfd, STDOUT_FILENO);
    close(clientfd);
    route();
    // cleanup
    fflush(stdout);
    shutdown(STDOUT_FILENO, SHUT_WR);
    close(STDOUT_FILENO);
    shutdown(clientfd, SHUT_RDWR);
    close(clientfd);
    clients[n] = -1;
}

// find header by name
char *request_header(const char* name) {
    header_t *h=reqhdr;
    while(h->name) {
        if (strcmp(h->name,name)==0) return h->value;
        h++;
    }
    return NULL;
}

void analyze_http(char* buf,int rcvd){
    buf[rcvd]='\0';
    // parse request line
    method = strtok(buf," \t\r\n");
    uri    = strtok(NULL," \t");
    prot   = strtok(NULL," \t\r\n");
    fprintf(stderr,"\x1b[32m + [%s] %s\x1b[0m\n",method,uri);

    // split query string
    if ((qs = strchr(uri,'?'))) *qs++ = '\0'; else qs = uri-1;

    // parse headers
    header_t *h = reqhdr;
    while(h<reqhdr+16){
        char *k=strtok(NULL,"\r\n: \t");
        if(!k) break;
        char *v=strtok(NULL,"\r\n");
        while(*v==' ') v++;
        h->name=k; h->value=v;
        fprintf(stderr,"[H] %s: %s\n",k,v);
        char *t=v+1+strlen(v);
        if(t[1]=='\r'&&t[2]=='\n') break;
        h++;
    }

    // content length
    char *cl = request_header("Content-Length");
    payload_size = cl?atoi(cl):0;

    // locate body
    char *body = strstr(buf,"\r\n\r\n");
    payload = body? body+4 : buf+rcvd-payload_size;

    // debug
    fprintf(stderr,"[DBG] payload_size=%d, payload='%.*s'\n",
            payload_size, payload_size, payload);
}
