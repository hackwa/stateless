#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <syslog.h>
#include <unistd.h>

#include "hircluster.h"

#define CHECK(X) if ( !X || X->type == REDIS_REPLY_ERROR ) { printf("Error\n"); exit(-1); }

redisReply *reply;

typedef struct _client {
    redisClusterContext *cluster_context;
    redisContext *local_context;
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

static client createClient(char *clusterid, char* unixsocket) 
{
    int j;
    client c = zmalloc(sizeof(struct _client));

    c->cluster_context = redisClusterConnect(clusterid,HIRCLUSTER_FLAG_NULL);
    c->local_context = redisConnectUnixNonBlock(unixsocket);

    if (c->cluster_context == NULL || c->cluster_context->err) {
        if (c->cluster_context) {
            printf("Connection error: %s\n", c->cluster_context->errstr);
            redisFree(c->cluster_context);
        } else {
            printf("Connection error: can't allocate redis context\n");
        }
        exit(1);
    }

    if (c->local_context == NULL || c->local_context->err) {
    if (c->local_context) {
        printf("Connection error: %s\n", c->local_context->errstr);
        redisFree(c->local_context);
    } else {
        printf("Connection error: can't allocate redis context\n");
    }
    exit(1);
    }
    return c;
}

void simpleCmd(redisContext *c, char* cmd)
{
    if(reply != NULL)
        freeReplyObject(reply);
    reply = redisCommand(c,cmd);
    CHECK(reply);    
}

void simplePipeline(redisContext *c , char **cmdlist, int num)
{
    if(reply != NULL)
    freeReplyObject(reply);
    int i;
    for (i=0; i<num; i++){
        redisAppendCommand(c,cmdlist[i]);
    }
    /* This writes the entire buffer to socket
       And waits for a single reply.*/ 
    redisGetReply(context,&reply);
}