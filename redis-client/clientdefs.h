#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <adapters/libevent.h>

//#include <hiredis.h>

#include "helper.h"

#define CHECK(X) if ( !X || X->type == REDIS_REPLY_ERROR ) { printf("Error in reply \n"); exit(-1); }
#define LOCAL_IFACE "enp2s0"

redisReply *reply;
redisReply *creply;
redisReply *areply;

typedef struct _client {
    redisClusterContext *cluster_context;
    redisContext *local_context;
    redisClusterAsyncContext *acc;
    sds obuf;
    char **randptr;         /* Pointers to :rand: strings inside the command buf */
    size_t randlen;         /* Number of pointers in client->randptr */
    size_t randfree;        /* Number of unused pointers in client->randptr */
    size_t written;         /* Bytes of 'obuf' already written */
    long long start;        /* Start time of a request */
    long long latency;      /* Request latency */
    int pending;            /* Number of pending requests (replies to consume) */
    int prefix_pending;     /* If non-zero, number of pending prefix commands. Commands
                               such as auth and select are prefixed to the pipeline of
                               benchmark commands and discarded after the first send. */
    int prefixlen;          /* Size in bytes of the pending prefix commands */
} *client;

client createClient(char *clusterid, char* unixsocket);
void simpleCmd(redisContext *c, char* cmd);
void simplePipeline(redisContext *c , char **cmdlist, int num);

void clusterCmd(redisClusterContext *cc, char* cmd);
void clusterPipeline(redisClusterContext *cc , char **cmdlist, int num);
void clusterAsyncCmd(redisClusterAsyncContext *acc, char **cmdlist, int *num);

void getCallback(redisClusterAsyncContext *acc, void *r, void *privdata);
void connectCallback(const redisAsyncContext *c, int status);
void disconnectCallback(const redisAsyncContext *c, int status);


char local_ipaddr[NI_MAXHOST];
void setIfAddr();

struct event_base *_ebase;
int accCounter;
typedef struct calldata
{
    redisClusterAsyncContext *acc;
    int count;
}calldata;

struct timeval  tv1, tv2;