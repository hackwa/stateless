#include "helper.h"

void bench_utils(redisContext *c, char* command);
void bench_utils_pipelined(redisContext *c, char* command);

void bench_set(redisContext *c)
{
    char *cmd = "SET cyy barrrr";
    bench_utils(c,cmd);
    bench_utils_pipelined(c,cmd);
}

void bench_get(redisContext *c)
{
    char *cmd = "GET cyy";
    bench_utils(c,cmd);
    bench_utils_pipelined(c,cmd);
}

void bench_ping(redisContext *c)
{
    char *cmd = "PING";
    bench_utils(c,cmd);
    bench_utils_pipelined(c,cmd);
}

void bench_cas(redisContext *c)
{
    char *cmd = "EVALSHA e8d8dee414b70a26bd55239c1c59d9fcb13fef29 1 1 6 6";
    bench_utils(c,cmd);
    bench_utils_pipelined(c,cmd);
}

void bench_utils(redisContext *c, char* command)
{
    long microseconds=0;
    int i;
    int numiters=1000;
    for(i=0; i<numiters; i++){
    gettimeofday(&tv1, NULL);
    reply = redisCommand(c,command);
    gettimeofday(&tv2, NULL);
    if(i==numiters-1)
    printf("%s: %s\n", command,reply->str);
    freeReplyObject(reply);
    if (tv2.tv_sec != tv1.tv_sec)printf("seconds not equal!\n");
    microseconds += (long) (tv2.tv_usec - tv1.tv_usec);
    }
    printf ("Average Total time = %d micro seconds\n",
         (int) microseconds/1000);
}

void bench_utils_pipelined(redisContext *c, char* command)
{
    long microseconds=0;
    int i;
    int numiters=100;
    for(i=0; i<numiters; i++){
    redisAppendCommand(c,command);
    }
    gettimeofday(&tv1, NULL);
    for(i=0; i<numiters; i++){
    redisGetReply(c,(void *)&reply);
//    freeReplyObject(reply);
    }
    gettimeofday(&tv2, NULL);
    printf("%s: %s\n", command,reply->str);
    freeReplyObject(reply);
    microseconds = (long) (tv2.tv_usec - tv1.tv_usec);
    printf ("Total time for %d pipelined requests = %d micro seconds\n",
         numiters,(int) microseconds);
}
